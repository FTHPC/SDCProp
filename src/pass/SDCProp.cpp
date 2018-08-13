#ifdef USE_OLD_ABI
#define _GLIBCXX_USE_CXX11_ABI 0
#endif

#include "SDCProp.h"
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/IntrinsicInst.h>

bool SDCProp::runOnModule(Module& Mod)
{
    M = &Mod;
     
    /* used for compiler log file in ASCII format */
    std::string LOG_FILE;
    LOG_FILE.reserve(1000);
    raw_string_ostream out(LOG_FILE);

    readConfig("SDCPROP.config");
    std::string srcFile = Mod.getModuleIdentifier();//"none.c";
    std::string stateFile = "foo";
	errs() << "FI CONFIG: " << flipitConfig << "\n";
    DynamicFaults* flipit = new DynamicFaults(""/*Fname*/, flipitConfig, 1e-7, -1, 1, 1, 1, 1, srcFile, stateFile, M);
    //DynamicFaults* flipit = new DynamicFaults(""/*Fname*/, flipitConfig, 0, -1, 1, 1, 1, 1, srcFile, stateFile, M);
    /*OLD*/ //DynamicFaults* flipit = new DynamicFaults(""/*Fname*/, flipitConfig, 1e-7, -1, 1, 1, 1, 1, srcFile, stateFile, M);

    /* construct a gold (GG) and faulty (_F) version of each function to modify
     * we construct a gold function to preserve the original for situations that
     * must call the original function */
    for (auto F = M->getFunctionList().begin(),
        E = M->getFunctionList().end(); F != E; F++) {
        //errs() << "!-------------------------------------!\n";
        if (F->begin() == F->end())
            continue;
        std::string name = demangle(F->getName().str());
        
        // don't make faulty version for system generated function
        if (std::find(dupFuncs.begin(), dupFuncs.end(), name) != dupFuncs.end())
        //if (std::find(singleFuncs.begin(), singleFuncs.end(), name) != singleFuncs.end())
        {    errs() << "@@@ Not creating GG_F version of func marked for duplication: " << name <<"\n";
            continue;
        }

        /*if (name == "__cxx_global_var_init" ||
            name == "_GLOBAL__sub_I_HPCCG.cpp"
            || name  == "std::basic_ostream<char, std::char_traits<char> >& std::operator<< <std::char_traits<char>"
            || name  == "std::ostream::operator<<"
            || name  == "std::basic_ostream<char, std::char_traits<char> >& std::endl<char, std::char_traits<char> >"
            || name  == "std::basic_ostream<char, std::char_traits<char> >& std::flush<char, std::char_traits<char> >"
            || name  == "std::basic_ios<char, std::char_traits<char> >::widen"
            || name  == "std::basic_ostream<char, std::char_traits<char> >& std::operator<< <std::char_traits<char> >"
            || name  == "std::ctype<char> const& std::__check_facet<std::ctype<char> >"
            || name  == "std::ctype<char>::widen"
            || name  == "std::basic_ios<char, std::char_traits<char> >::setstate"
            || name  == "std::char_traits<char>::length"
            || name  == "std::operator|"
            || name  == "std::basic_ios<char, std::char_traits<char> >::rdstate"
            || name  == "_GLOBAL__sub_I_HPCCG.cpp")
        {    errs() << "@@@@@@@@@@: " << name <<"\n";
            continue; }
        */
        errs() << name << "\n";
        /* for instrumentation of the main function the roles are reversed. */
        if (demangle(F->getName().str()) != "main") {
            goldFunc.push_back(createFaultyFunction(F, "GG"));
            newFunc.push_back(createFaultyFunction(goldFunc.back(), "_F", 2));
        }
        else {
            newFunc.push_back(F);
            goldFunc.push_back(createFaultyFunction(F, "_F"));
        }
        faultyFuncs.push_back(newFunc.back());
        assert(newFunc.back()->getParent() != NULL && "no parent for newF\n");
    }

    //errs() << "\n\n*************** Instrumenting *****************\n";
    int ID_start = initInstrumentation();
    copyAndUpdateGlobals();
    BB_ID = ID_start;
    
    /* modify each function */
    for (unsigned i = 0; i < newFunc.size(); i++) {
        auto NewF = newFunc[i];
        auto GoldF = goldFunc[i];
        auto Fname = demangle(NewF->getName().str());
        isMain = (Fname == "main");
        std::vector<BasicBlock*> BBs;
        std::vector<BasicBlock*> NewFBBs;
        std::vector<Value*> args;
        unsigned k = 0;


        Fname = Fname.substr(0,Fname.size()-4);
        errs() << "Function Name: " << demangle(NewF->getName().str());

        errs() << "\n\n----------- INSTRUMENT -----------------\n\n";
        
        // Create lists of gold(BBs) and faulty(NewFBBs) BBs then loop over all BB
        // and add instructions from NewF to F
        for (auto BB = GoldF->begin(), e = GoldF->end(); BB !=e; BB++) {
            BBs.push_back(BB);
        }
        for (auto BB = NewF->begin(), e = NewF->end(); BB !=e; BB++) {
            NewFBBs.push_back(BB);
        }
        assert(BBs.size() == NewFBBs.size()
            && "Unequal number of basic blocks.");

        // Copy instructions from faulty BB to gold BB
        // add the instrumentation code
        nAllocas = 0;
        for (unsigned i=0; i<BBs.size(); i++) {
            instrumentBB(BBs[i], NewFBBs[i]);
        }

        
         errs() << "\n\n--------------- Logging gbl vars used in funtion ------------------\n";
        // insert logging of gbl vars used in this function
       if(gblVar.find(NewF->getName()) != gblVar.end()) { 
         for (auto gbl : *(gblVar[NewF->getName()])) {
            auto begin = NewFBBs[0]->begin();
            assert(gbl.first != NULL && "1st is null\n");
            assert(gbl.second != NULL && "2nd is null\n");
            args.push_back(new BitCastInst(gbl.second, Type::getInt8PtrTy(getGlobalContext()),"bc", begin));
            args.push_back(new BitCastInst(gbl.first, Type::getInt8PtrTy(getGlobalContext()),"bc", begin));
            auto nBytes = Layout->getTypeStoreSize(gbl.first->getType()->getContainedType(0));
            args.push_back(ConstantInt::get(i64Ty, nBytes));
            CallInst::Create(logGbl, args, "", begin);
            args.erase(args.begin(), args.end());
            }
        }
        /********** Inject Faults in the faulty function's insructions *********/
#ifdef FLIPIT
        //DynamicFaults* flipit = new DynamicFaults(Fname, flipitConfig/*"FlipIt.config"*/, 1e-3, -1, 1, 1, 1, 1, M);
        for (auto I : flipitInsts) {
            //I->printAsOperand(out, true, M);
            BasicBlock::iterator BI = I;
            BI++;
            flipit->corruptInstruction(I); //printAsOperand(out, true, M);
            /* fix issue
             *  ptr  = call malloc
             *  corrupt(ptr)
             *  __LOG_MALLOC(ptr) <-- could be faulty need to log before corruption
             */
            //TODO: only apply this fix for functions we inject into!!!
            if (isa<CallInst>(I) && demangle(I->getParent()->getParent()->getName().str()) != "main") {
                auto fName = demangle(dyn_cast<CallInst>(I)->getCalledFunction()->getName().str());
                if (fName == "malloc" 
                    || fName.find("operator new") != std::string::npos) {
                    BI->setOperand(0, I);
                }
                else if (fName == "calloc") {
                    BI++; // Why?
                    BI->setOperand(0, I);
                }
                else if (fName == "realloc") {
                    BI->setOperand(0, I);
                }
            }
        }
        flipitInsts.erase(flipitInsts.begin(), flipitInsts.end());
    
        // move all phi instructions to top of BB
        for (auto phi : phiInsts) {
            auto BB = phi->getParent();
            auto i = BB->getFirstNonPHI();
            phi->moveBefore(i);
        }
        phiInsts.erase(phiInsts.begin(), phiInsts.end());

#endif
        // remove uses of GoldF's arguments to allow for successful erasing from module
         errs() << "\n\n--------------- RM args from function for sucessful removal ------------------\n";
        for (auto a  = NewF->getArgumentList().begin(), e =  NewF->getArgumentList().end(); a !=e; a++) {
            args.push_back(a);
        }
        for (auto a  = GoldF->getArgumentList().begin(), e =  GoldF->getArgumentList().end(); a !=e; a++) {
            a->replaceAllUsesWith(args[k++]);
        }
        errs() << "erase GoldF\n";
        GoldF->eraseFromParent();

         errs() << "\n\n--------------- Log CMD line args ------------------\n";
        // log values in argv for main function
        if (isMain) {
            auto a = NewF->getArgumentList().begin();
            std::vector<Value*> args { a, ++a};
            CallInst::Create(logCmdArgs, args, "", NewF->begin()->begin());
        }
        //mark the start of each function that is instrumented
        args.erase(args.begin(), args.end());
        if (!isMain){
            CallInst::Create(startFunc, args, "", NewFBBs[0]->begin());
        } else {
            CallInst::Create(startFunc, args, "",  NewF->begin()->begin());
        }
    }


    delete flipit;
    errs() << "\n******* FINAL DUMP OF MODULE *****\n";
    createFileNameGlobal("ukwn.c");
    M->dump();
    //errs() << out.str();
    return true;
    }

int SDCProp::initInstrumentation()
{
    int numBBs = 0;
    Layout = new DataLayout(M);
    createMask();

    //readConfig("SDCPROP.config");

    // find each function in the file
    for (auto f = M->getFunctionList().begin(), 
        e = M->getFunctionList().end(); f != e; f++) {
        std::string name = demangle(f->getName().str());
        // Gold Stores
        if (name.find("__STORE_INT") != std::string::npos) {
            storeInt = &*f;
        }
        else if (name.find("__STORE_FLOAT") != std::string::npos) {
            storeFloat = &*f;
        }
        else if (name.find("__STORE_DOUBLE") != std::string::npos) {
            storeDouble = &*f;
        }
        else if (name.find("__STORE_PTR") != std::string::npos) {
            storePtr = &*f;
        }
        // Loads
        else if (name.find("__LOAD_INT") != std::string::npos) {
            loadInt = &*f;
        }
        else if (name.find("__LOAD_FLOAT") != std::string::npos) {
            loadFloat = &*f;
        }
        else if (name.find("__LOAD_DOUBLE") != std::string::npos) {
            loadDouble = &*f;
        }
        else if (name.find("__LOAD_PTR") != std::string::npos) {
            loadPtr = &*f;
        }
        // various other instrumentation functions
        else if (name.find("__DUMMY_FREE") != std::string::npos) {
            dummyFree = &*f;
        }
        else if (name.find("__LOG_AND_RELATE_ALLOCA") != std::string::npos) {
            logAlloca = &*f;
        }
        else if (name.find("__REMOVE_ALLOCA") != std::string::npos) {
            rmAlloca = &*f;
        }
        else if (name.find("__LOG_AND_RELATE_MALLOC") != std::string::npos) {
            logMalloc = &*f;
        }
        else if (name.find("__LOG_AND_RELATE_REALLOC") != std::string::npos) {
            logRealloc = &*f;
        }
        else if (name.find("__CHECK_CMP") != std::string::npos) {
            checkCmp = &*f;
        }
        else if (name.find("__LOG_CMD_ARGS") != std::string::npos) {
            logCmdArgs = &*f;
        }
        else if (name.find("__GET_FAULTY_VALUE") != std::string::npos) {
            relateAlloca = &*f;
        }
        else if (name.find("__LOG_MALLOCS_IN_CALL") != std::string::npos) {
            startCallLogging = &*f;
        }
        else if (name.find("__RELATE_MALLOCS_IN_CALL") != std::string::npos) {
            relateCallMallocs = &*f;
        }
        else if (name.find("__LOG_AND_RELATE_GLOBAL") != std::string::npos) {
            logGbl = &*f;
        }
        else if (name.find("__START_FUNCTION") != std::string::npos) {
            startFunc = &*f;
        }
        else if (name.find("__LOAD_VECTOR_ELEMENT") != std::string::npos) {
            loadVecElm = &*f;
        }
        else if (name.find("__STORE_VECTOR") != std::string::npos) {
            storeVecElm = &*f;
        }
        else if (name.find("GG_F") != std::string::npos) {
           faultyFuncs.push_back(f); 
        }
        numBBs += f->size();
        // prevents compiler issuing ud2 instruction when calling conventions
        if (f->hasFnAttribute(llvm::Attribute::NoAlias)) {
            f->removeFnAttr(llvm::Attribute::NoAlias);
        }
    }

    assert(storeInt != NULL && storeFloat != NULL
        && storeDouble != NULL && storePtr != NULL
        && loadInt != NULL && loadFloat != NULL
        && loadDouble != NULL && loadPtr != NULL
        && dummyFree != NULL && logAlloca != NULL
        && rmAlloca != NULL && checkCmp != NULL
        && relateAlloca != NULL && logMalloc != NULL
        && logCmdArgs != NULL && startCallLogging != NULL
        && relateCallMallocs != NULL && logGbl != NULL
        && startFunc != NULL && logRealloc != NULL
        && loadVecElm != NULL && storeVecElm != NULL
        && "Unable to find instrumention functions!");

    int idStart = getStartID(numBBs, "./BBID.txt");
    assert(idStart >= 0 && "BB id from file is negative!");
    return idStart;
}

void SDCProp::readConfig(std::string filename)
{
    ifstream infile;
    string line;
    infile.open(filename.c_str());

    // read duplicate funcs
    getline(infile, line); // DUPLICATE:
    getline(infile, line); // function to duplicate

    while (line != "SINGLE:" && infile) {
        dupFuncs.push_back(line);
        getline(infile, line); // another func name
    }

    // non duplicate functions
    while (line != "FLIPIT:" && infile) {
        singleFuncs.push_back(line);
        getline(infile, line); // another func name
    }

    // read Flipit config name
    getline(infile, flipitConfig);
    infile.close();
}

int SDCProp::getStartID(int size, const char* path)
{
    const char* lockName = "./SDCPropLock.txt";
    int lock, IDs, ret = 0, num = 0;
    char buffer[20];
    do {
        lock = tryLock(lockName);
    } while(lock == -1);

    // TODO: Replace with code from flipit
    IDs = open(path, O_RDWR | O_CREAT, 0666);
    read(IDs, buffer, 20);
    sscanf(buffer, "%d", &ret);
    num = ret + size;
    lseek(IDs, 0, SEEK_SET);
    memset(buffer, '\0', 20);
    snprintf(buffer, 20, "%d", num);
    write(IDs, buffer, 20);
    close(IDs);
    releaseLock(lock, lockName);
    
    return ret;
}

bool SDCProp::tryLock(const char* lockName)
{
    //int lock = open(lockName, O_RWDR | O_CREAT, 0666);
    int lock = open(lockName, O_RDWR | O_CREAT | O_EXCL, 0666);
    if(lock >= 0 && flock( lock, LOCK_EX | LOCK_NB ) < 0)
    {
        close(lock);
        lock = -1;
    }
    return lock;
}

void SDCProp::releaseLock(int lock, const char* lockName)
{
    if (lock < 0)
        return;
    remove(lockName);
    close(lock);
}

void SDCProp::copyAndUpdateGlobals()
{
    // loop over all global variables and add references to global var map for each function that uses them 
    std::vector<GlobalVariable*> gbls;
    for (auto I= M->getGlobalList().begin(), E=M->getGlobalList().end(); I!=E; I++) {
        gbls.push_back(I);
    }
    for (auto I : gbls) {
        //DEBUGerrs() << "Workingon inst: " << *I << "\n";
        //duplicate
        GlobalVariable* GV = NULL;
        if (I->getName().find("__F") == std::string::npos &&
                I->hasInitializer()
                && I->getName().find("llvm.global_ctors") == std::string::npos) {
            GV = new GlobalVariable(*M, I->getType()->getElementType(),
                I->isConstant(), I->getLinkage(), (Constant*) nullptr, I->getName()+"__F", (GlobalVariable*) nullptr,
                I->getThreadLocalMode(), I->getType()->getAddressSpace());
            //if (I->hasInitializer())
                GV->setInitializer(I->getInitializer());
        }

        std::vector<User*> userVec;
        // add to map based on use
        for (auto user : I->users()) {
            if (isa<Instruction>(user)) {
                auto name = dyn_cast<Instruction>(user)->getParent()->getParent()->getName();
                if (gblVar.find(name) == gblVar.end())
                    gblVar.insert(std::pair<std::string, std::vector<std::pair<GlobalVariable*, GlobalVariable*>>*>(name, new std::vector<std::pair<GlobalVariable*, GlobalVariable*>> ()));
                // cache users of I
                if (name.find("GG_F") != std::string::npos) {
                    userVec.push_back(user);
                }
                if (std::find_if(gblVar[name]->begin(), gblVar[name]->end(), [&](const std::pair<GlobalVariable*, GlobalVariable*>& elm) { return elm.first == I;})  == gblVar[name]->end()) {
                //if (std::find(gblVar[name]->begin(), gblVar[name]->end(), I) == gblVar[name]->end()) {
                    //DEBUGerrs() << "Adding to hash table\n";
                    if (GV != NULL) {
                        gblVar[name]->push_back(std::make_pair(I, GV));
                    }
                }
            }
        }
        // replase fauly uses with faulty global
        for (auto user : userVec) {
            if (GV != NULL)
                user->replaceUsesOfWith(I, GV);
        }
    }
}

Function* SDCProp::createProtoType(Function* F, std::string suffix, int nRepeat)
{
    FunctionType* FTy;
    std::vector<Type*> ArgTypes;

    //add the arg types
    for (auto i=0; i<nRepeat; i++)
        for (auto I = F->arg_begin(), E = F->arg_end(); I != E; ++I)
            ArgTypes.push_back(I->getType());

    // Create a new function type...
    auto retType = F->getReturnType();
    if (!retType->isVoidTy())
        retType = ArrayType::get(retType, 2);
    FTy = FunctionType::get(retType, ArgTypes, F->getFunctionType()->isVarArg());
    
    auto NewF = Function::Create(FTy, Function::ExternalLinkage, F->getName()+suffix, M);
    
    // force call by stack
    AttrBuilder B; 
    B.addAttribute(llvm::Attribute::NoAlias);
    B.addAttribute(llvm::Attribute::InAlloca);
    NewF->removeAttributes(0, AttributeSet::get(getGlobalContext(), 0, B));
    
    for (auto I = NewF->arg_begin(), E = NewF->arg_end(); I != E; ++I) {
        AttrBuilder ab;
        ab.addAttribute(llvm::Attribute::NoAlias);
        ab.addAttribute(llvm::Attribute::InAlloca);
        I->removeAttr(AttributeSet::get(getGlobalContext(), 0, ab));
    }
    
    return NewF;
}

Function* SDCProp::createFaultyFunction(Function* F, std::string suffix, int nRepeat)
{
    ValueToValueMapTy VMap;
    SmallVector<ReturnInst*, 8> Returns;  // Ignore returns cloned
    FunctionType* FTy;
    Function* NewF;
    Function::arg_iterator DestI;
    std::vector<Type*> ArgTypes;

    // Code addapted from CloneFunction.cpp:216
    // The user might be deleting arguments to the function by specifying them in
    // the VMap.  If so, we need to not add the arguments to the arg ty vector
    for (auto I = F->arg_begin(), E = F->arg_end(); I != E; ++I)
        if (VMap.count(I) == 0)  // Haven't mapped the argument to anything yet?
            ArgTypes.push_back(I->getType());
    for (auto i=1; i<nRepeat; i++)
        for (auto I = F->arg_begin(), E = F->arg_end(); I != E; ++I)
            ArgTypes.push_back(I->getType());
    
    auto retType = F->getReturnType();
    if (!retType->isVoidTy() && nRepeat == 1)
        retType = ArrayType::get(retType, 2);
    // Create a new function type...
    FTy = FunctionType::get(retType, ArgTypes, F->getFunctionType()->isVarArg());

    //FTy = FunctionType::get(F->getFunctionType()->getReturnType(),
    //                                 ArgTypes, F->getFunctionType()->isVarArg()); 
    // Create the new function...
    NewF = Function::Create(FTy, F->getLinkage(), F->getName()+suffix);
    // Loop over the arguments, copying the names of the mapped arguments over...
    DestI = NewF->arg_begin();
    for (auto I = F->arg_begin(), E = F->arg_end(); I != E; ++I)
        if (VMap.count(I) == 0) {   // Is this argument preserved?
             DestI->setName(I->getName()); // Copy the name over...
            VMap[I] = DestI++;        // Add mapping to VMap
        }
     auto newI = NewF->arg_end();
    for (auto I = F->arg_end(), E = F->arg_begin(); I!=E; --I) {
        if (I != F->arg_end())
            newI->setName(I->getName()+suffix);
        newI--;
    }
    if (F->arg_begin() != F->arg_end())
        newI->setName(F->arg_begin()->getName()+suffix);
    //if (ModuleLevelChanges)
    //    CloneDebugInfoMetadata(NewF, F, VMap);


    // Clone into the new function and add it to the current module
    CloneFunctionInto(NewF, F, VMap, false, Returns, suffix.c_str(), NULL);
    AttrBuilder B; // = new AttrBuilder();
    B.addAttribute(llvm::Attribute::NoAlias);
    B.addAttribute(llvm::Attribute::InAlloca);
    NewF->removeAttributes(0, AttributeSet::get(getGlobalContext(), 0, B));
    for (auto I = NewF->arg_begin(), E = NewF->arg_end(); I != E; ++I) {
        AttrBuilder ab;
        ab.addAttribute(llvm::Attribute::NoAlias);
        ab.addAttribute(llvm::Attribute::InAlloca);
        I->removeAttr(AttributeSet::get(getGlobalContext(), 0, ab));
    }
    //A.addAttribute(getGlobalContext(), 0, llvm::Attribute::NoAlias);
    
    for(auto I = NewF->arg_begin(), E = newI; I != E; ++I) {
        I->replaceAllUsesWith(newI);
        newI++;
    }
    
    F->getParent()->getFunctionList().push_front(NewF);
//    NewF->removeFnAttr(llvm::Attribute::NoAlias);
    return NewF;
}

void SDCProp::createMask()
{
    loadMask = new GlobalVariable(*M, i64Ty, false,
                            GlobalValue::CommonLinkage, 0, "mask");
    // Global Variable Definitions
    loadMask->setInitializer(ConstantInt::get(i64Ty, 0));
    
    // float
    floatMask = new GlobalVariable(*M, floatTy, false,
                            GlobalValue::CommonLinkage, 0, "flt_mask");
    floatMask->setInitializer(ConstantFP::get(floatTy, 0.0));
    
    // double
    doubleMask = new GlobalVariable(*M, doubleTy, false,
                            GlobalValue::CommonLinkage, 0, "dbl_mask");
    doubleMask->setInitializer(ConstantFP::get(doubleTy, 0.0));
    
    // pointer
    ptrMask = new GlobalVariable(*M, ptrTy, false,
                            GlobalValue::CommonLinkage, 0, "ptr_mask");
    Type* i8Ty = Type::getInt8Ty(getGlobalContext());
    ptrMask->setInitializer( ConstantPointerNull::get( PointerType::get(i8Ty, 0)));
}

std::string SDCProp::demangle(std::string name)
{
    int status;
    std::string demangled;
    char* tmp = abi::__cxa_demangle(name.c_str(), NULL, NULL, &status);
    if (tmp == NULL)
        return name;
    
    demangled = tmp;
    free(tmp);
    
    /* drop the parameter list as we only need the function name */
    return demangled.find("(") == string::npos ? demangled : demangled.substr(0, demangled.find("("));
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

bool SDCProp::instrumentBB(BasicBlock*& oldBB, BasicBlock* newBB)
{
    assert(oldBB != NULL && newBB != NULL && "BB to instrument is NULL!\n");
    assert(oldBB->size() == newBB->size() && "BB's sizes are not equal.");
    int numMemOps = 0;
    std::map<Value*, Value*> replace;
    std::vector<Instruction*> toErase;
    std::string nameBB = newBB->getName().str();
    if (nameBB.find("lpad") != std::string::npos
        || nameBB.find("ehcleanup") != std::string::npos 
        || nameBB.find("eh.resume") != std::string::npos) {
            //DEBUGerrs() << "Returning flase for BB " << nameBB << "\n";
        for (auto newI = newBB->begin(), E = newBB->end(); newI != E; newI++) {
            auto oldI = oldBB->begin();
            oldI->dropAllReferences();
            oldI->removeFromParent();
        }
        if (oldBB->getSinglePredecessor() != NULL)
            oldBB->removePredecessor(oldBB->getSinglePredecessor());
        oldBB->dropAllReferences();
        return false;
    }

    //DEBUGerrs() << "Working on BB " << nameBB << "\n";
    
    // Copy all insts from oldBB to newBB, modify if lds, sts, malloc, alloc, 
    // and add copied insts to FlipIt insts
    for (auto newI = newBB->begin(), E = newBB->end(); newI != E; newI++) {
    
        MDNode* N;
        N = newI->getMetadata("dbg");
        int line = -1;
        std::string fname = "NOPE.c";
        if (N != NULL) {
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR <= 6
            DILocation Loc(N);
            fname = Loc.getDirectory().str() + "/" + Loc.getFilename().str();
            line = Loc.getLineNumber();
                createFileNameGlobal(fname);
#elif LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 6
            DILocation* Loc = oldI->getDebugLoc();
            fname = Loc->getDirectory().str() +"/" + Loc->getFilename().str();
            line = Loc->getLineNumber();
                createFileNameGlobal(fname);
#endif
        }
        errs() << fname << ":" << line << "\n";
        auto oldI = oldBB->begin();
        auto Inext = newI; Inext++;
        oldI->removeFromParent();
        
        if (isa<StoreInst>(newI) == true) {
            numMemOps++;
            newBB->getInstList().insert(newI, oldI);
            if (instrumentStore(newI, oldI)) {
                (isMain ? oldI : newI)->eraseFromParent(); // frees memory
            }
        }
        else if (isa<LoadInst>(newI) == true) {
            numMemOps++;
            newBB->getInstList().insert(newI, oldI);
            instrumentLoad(newI, oldI, replace);
        }
        else if (isa<PHINode>(newI) == true) {
            auto newPhi = dyn_cast<PHINode>(newI);
            auto oldPhi = dyn_cast<PHINode>(oldI);
            phiInsts.push_back(newPhi);
            phiInsts.push_back(oldPhi);
        
            for (unsigned i = 0; i < oldPhi->getNumIncomingValues(); i++)
                oldPhi->setIncomingBlock(i, newPhi->getIncomingBlock(i));
            newBB->getInstList().insert(newPhi, oldPhi);
            flipitInsts.push_back(newI);
        }
        else if (isa<ReturnInst>(newI) == true) {
            if (newI->getNumOperands() != 0
                    && !newI->getOperand(0)->getType()->isVoidTy()
                    && newBB->getParent()->getName() != "main")
            {
                auto insert = InsertValueInst::Create(UndefValue::get(ArrayType::get(newI->getOperand(0)->getType(), 2)),
                        newI->getOperand(0), {0}, "insert", newI);
                insert = InsertValueInst::Create(insert, oldI->getOperand(0), {1}, "insert", newI);
                newI->setOperand(0, insert);
            }

            //if (nAllocas > 0)
                removeAllocas(nAllocas, newI);
            newBB->getInstList().insert(newI, oldI);
            toErase.push_back(oldI);
        }
        else if (isa<CallInst>(newI) == true) {
            newBB->getInstList().insert(newI, oldI);
            instrumentCall(newI, oldI, Inext, replace, toErase);
        }
        else if (isa<AllocaInst>(newI) == true) {
            newBB->getInstList().insert(newI, oldI);
            handleAlloc(newI, oldI, Inext);
            nAllocas++;
        }
        else if (isa<CmpInst>(newI) == true) {
            newBB->getInstList().insert(newI, oldI);
            flipitInsts.push_back(newI);
            instrumentCompare(newI, oldI, Inext);
        }
        else if (isa<BranchInst>(newI) == true) {
            if (!newI->getOperand(0)->getType()->isLabelTy())
                newI->setOperand(0, oldI->getOperand(0));
            newBB->getInstList().insert(newI, oldI);
            toErase.push_back(oldI);
        }
        else if (isa<UnreachableInst>(newI)) {
            newBB->getInstList().insert(newI, oldI);
            toErase.push_back(newI);
        }
        else if (isa<SwitchInst>(newI)) {
            handleSwitch(newI, oldI, toErase);
        }
        else if (isa<InvokeInst>(newI)) {
            newBB->getInstList().insert(newI, oldI);
            handleInvoke(newI, oldI, Inext, toErase);
        }
        else if (isa<ResumeInst>(newI)) {
            newBB->getInstList().insert(newI, oldI);
            toErase.push_back(newI);
        }
        else { // binop, GEP, etc.
            newBB->getInstList().insert(newI, oldI);
            flipitInsts.push_back(newI);
        }
        
        //point to current instruction before the original next
        newI = --Inext;
    }


    // fix up load instructions by replacing the use of the original
    // load with that of the function call
    for (auto load : replace) {
        load.first->replaceAllUsesWith(load.second);
    }
    
    // erase the instruction that are no longer needed
    for (auto I : toErase) {
        I->eraseFromParent();
    }
    return true;
}

bool SDCProp::instrumentStore(Instruction* newI, Instruction* oldI)
{
    if (isMain) {
        std::swap(newI, oldI);
    }
    Value* data = newI->getOperand(0);
    Value* ptr = newI->getOperand(1);
    Value* origData = oldI->getOperand(0);
    Value* origPtr = oldI->getOperand(1);
    CallInst* call = NULL;
    std::vector<Value*> args(4);
    /*
        args[0] = faulty ptr
        args[1] = faulty value
        args[2] = orig ptr
        args[3] = orig value
        args[4] = type (integer types only)
        args[5] = mask (integer types only)
    */
    
    // convert faulty address to i64
    args[0] = new PtrToIntInst(ptr, i64Ty, "ptr2intCpy", oldI);
    args[2] = new PtrToIntInst(origPtr, i64Ty, "ptr2intOrig", oldI);
    
    if(data->getType()->isIntegerTy()) {
        int nBytes = data->getType()->getIntegerBitWidth()/8;
        
        if (nBytes != 8) {
            args[1] = new ZExtInst(data, i64Ty, "zXIntCpy", oldI);
            args[3] = new ZExtInst(origData, i64Ty, "zXIntOrig", oldI);
        }
        else {
            args[1] = data;
            args[3] = origData;
        }
        
        args.push_back(ConstantInt::get(i32Ty, nBytes));
        LoadInst* mask = new LoadInst(loadMask, "loadMaskS", oldI);
        args.push_back(mask);
        flipitInsts.push_back(mask);
        copyMetadata(mask, newI);
         
        call = CallInst::Create(storeInt, args, "", oldI);
    }
    else if (data->getType()->isFloatTy()) {
        args[1] = data;
        args[3] = origData;
        LoadInst* mask = new LoadInst(floatMask, "fltMaskS", oldI);
        args.push_back(mask);
        flipitInsts.push_back(mask);
        copyMetadata(mask, newI);
        
        call = CallInst::Create(storeFloat, args, "", oldI);
        //flipitInsts.push_back(call); 
    }
    else if (data->getType()->isDoubleTy()) {
        args[1] = data;
        args[3] = origData;
        LoadInst* mask = new LoadInst(doubleMask, "dblMaskS", oldI);
        args.push_back(mask);
        flipitInsts.push_back(mask);
        copyMetadata(mask, newI);
        
        call = CallInst::Create(storeDouble, args, "", oldI);
        //flipitInsts.push_back(call); 
    }
    else if (data->getType()->isPointerTy()) {
        args[1] = new PtrToIntInst(data, i64Ty, "ptr2int_storeCpy", oldI);
        args[3] = new PtrToIntInst(origData, i64Ty, "ptr2int_storeOrig", oldI);
        LoadInst* mask = new LoadInst(ptrMask, "ptrMaskS", oldI);
        args.push_back(mask);
        flipitInsts.push_back(mask);
        copyMetadata(mask, newI);
        
        call = CallInst::Create(storePtr, args, "", oldI);
        //flipitInsts.push_back(call); 
    }
    else if (data->getType()->isVectorTy()) {
        LoadInst* mask = NULL; 
        //set the mask and TODO: set type
        auto elmTy = data->getType()->getScalarType();
        auto type = UNKNOWN;
        if (elmTy->isIntegerTy()) {
            mask = new LoadInst(loadMask, "loadMask", oldI);
            if (elmTy->isIntegerTy(8))
                type = INT8;
            else if (elmTy->isIntegerTy(16))
               type = INT16;
            else if (elmTy->isIntegerTy(32))
               type = INT32;
            else if (elmTy->isIntegerTy(64))
               type = INT64;
        }
        else if (elmTy->isFloatTy()) {
            mask = new LoadInst(floatMask, "f32Msk", oldI);
            type = FLOAT;
        }
        else if (elmTy->isDoubleTy()) {
            mask = new LoadInst(doubleMask, "f32Msk", oldI);
            type = DOUBLE;
        }
        else if (elmTy->isPointerTy()) {
            mask = new LoadInst(ptrMask, "ptrMsk", oldI);
            type = PTR;
        }
        
        if (mask != NULL) { // break out the vector into a series of loads
            int nElms = dyn_cast<VectorType>(data->getType())->getNumElements();
            args[1] = args[2];
            args[2] = ConstantInt::get(i32Ty, nElms);
            args[3] = ConstantInt::get(i32Ty, type); // TODO <--- check type for right name
            args.push_back(mask);
            
            call = CallInst::Create(storeVecElm, args, "", oldI);
            flipitInsts.push_back(mask);
            copyMetadata(mask, newI);
        }
    }
    else 
        return false;
    copyMetadata(call, newI);
    if (isMain) {
        std::swap(newI, oldI);
    }
    return true;
}

//bool SDCProp::instrumentLoad(Instruction* newI, Instruction* oldI, std::map<Value*, Value*>& replace)
bool SDCProp::instrumentLoad(Instruction* fI, Instruction* gI, std::map<Value*, Value*>& replace)
{
    if (isMain) {
        std::swap(fI, gI);
    }
    LoadInst* mask = NULL;
    BasicBlock::iterator Inext(gI); Inext++;
    Value* data = fI;
    Value* ptr = fI->getOperand(0);
    Value* origData = gI;
    Value* origPtr = gI->getOperand(0);
    CallInst* call;
    std::vector<Value*> args(3);
    /*
        args[0] = faulty ptr
        args[1] = orig ptr
        args[2] = orig value
        args[3] = mask
        args[4] = type (only for integer)
    */
    
    // convert faulty address to i64
    args[0] = new PtrToIntInst(ptr, i64Ty, "ptr2intCpy", Inext);
    args[1] = new PtrToIntInst(origPtr, i64Ty, "ptr2intOrig", Inext);
    
    if(data->getType()->isIntegerTy()) {
        int nBytes = data->getType()->getIntegerBitWidth()/8;
        if (nBytes != 8) {
            args[2] = new ZExtInst(origData, i64Ty, "zXIntorig", Inext);    
        } 
        else {
            args[2] = origData;
        }
        mask = new LoadInst(loadMask, "loadMask", Inext);
        args.push_back(mask);
        args.push_back(ConstantInt::get(i32Ty, nBytes));
        call = CallInst::Create(loadInt, args, "loadInt", Inext);

        if (nBytes != 8) {
            replace[fI] = new TruncInst(call, data->getType(), "trucInt", Inext);
        }
        else {
            replace[fI] = call;
        }
    }
    else if (data->getType()->isFloatTy()) {
        args[2] = origData;
        mask = new LoadInst(floatMask, "f32Msk", Inext);
        args.push_back(mask);
        call = CallInst::Create(loadFloat, args, "ldf32", Inext);
        replace[fI] = call;
    }
    else if (data->getType()->isDoubleTy()) {
        args[2] = origData;
        mask = new LoadInst(doubleMask, "f32Msk", Inext);
        args.push_back(mask);
        call = CallInst::Create(loadDouble, args, "ldf64", Inext);
        replace[fI] = call;
    }
    else if (data->getType()->isPointerTy()) {
        args[2] = new PtrToIntInst(origData, i64Ty, "p2i_oData", Inext);
        mask = new LoadInst(ptrMask, "ptrMsk", Inext);
        args.push_back(mask);
        
        call = CallInst::Create(loadPtr, args, "ldPtr", Inext);
        replace[fI] = new IntToPtrInst(call, gI->getType(), "i2p_ld", Inext);
    }
    else if (data->getType()->isVectorTy()) {
        
        //set the mask and TODO: set type
        auto elmTy = data->getType()->getScalarType();
        auto type = UNKNOWN;
        if (elmTy->isIntegerTy()) {
            mask = new LoadInst(loadMask, "loadMask", Inext);
            if (elmTy->isIntegerTy(8))
                type = INT8;
            else if (elmTy->isIntegerTy(16))
               type = INT16;
            else if (elmTy->isIntegerTy(32))
               type = INT32;
            else if (elmTy->isIntegerTy(64))
               type = INT64;
        }
        else if (elmTy->isFloatTy()) {
            mask = new LoadInst(floatMask, "f32Msk", Inext);
            type = FLOAT;
        }
        else if (elmTy->isDoubleTy()) {
            mask = new LoadInst(doubleMask, "f32Msk", Inext);
            type = DOUBLE;
        }
        else if (elmTy->isPointerTy()) {
            mask = new LoadInst(ptrMask, "ptrMsk", Inext);
            type = PTR;
        }
        assert(mask != NULL && "Fault Injection mask is NULL\n");

        // break out the vector into a series of loads
        int nElms = dyn_cast<VectorType>(data->getType())->getNumElements();
        args[2] = ConstantInt::get(i32Ty, 0);
        args.push_back( ConstantInt::get(i32Ty, type)); // TODO <--- check type for right name
        args.push_back(mask);
        auto ld = CallInst::Create(loadVecElm, args, "ldVecElm", Inext);
        //auto ins = InsertValueInst::Create(UndefValue::get(data->getType()), ld, 0, "ei", Inext);
        auto ins = InsertElementInst::Create(UndefValue::get(data->getType()), ld, args[2], "ei", Inext);
        for (auto i = 1; i < nElms; i++) {  // for other elements
            args[2] = ConstantInt::get(i32Ty, i);
            if (i > 0)
                args[args.size()-1] = ConstantFP::get(Type::getDoubleTy(getGlobalContext()), 0);
            
            ld = CallInst::Create(loadVecElm, args, "ldVecElm", Inext);
            ins = InsertElementInst::Create(ins, ld, args[2], "ei", Inext);
        }
        replace[fI] = ins;
    }
    else 
        return false;
    
    //enumerate fault site and add dbg info
    flipitInsts.push_back(mask);
    copyMetadata(mask, fI);

    // update dependent insts and remove undeeded load
    copyMetadata(dyn_cast<Instruction>(replace[fI]), fI);
    fI->replaceAllUsesWith(replace[fI]);
    fI->eraseFromParent();
    replace.erase(fI);
    if (isMain) {
        std::swap(fI, gI);
    }

    return true;
}

bool SDCProp::handleAlloc(Instruction* newI, Instruction* oldI, Instruction* Inext)
{
    int type = UNKNOWN;
    Type* allocTy = NULL;
    if (!oldI->getType()->isVoidTy()) {
        allocTy = oldI->getType()->getContainedType(0);
        if (allocTy != NULL && allocTy->isArrayTy()) {
            allocTy = allocTy->getContainedType(0);
        }
    }
    if (allocTy == NULL) {
        type = UNKNOWN;
    } else if (allocTy->isIntegerTy()) {
       if (allocTy->isIntegerTy(8))
           type = INT8;
       else if (allocTy->isIntegerTy(16))
           type = INT16;
       else if (allocTy->isIntegerTy(32))
           type = INT32;
       else if (allocTy->isIntegerTy(64))
           type = INT64;
    } else if (allocTy->isFloatTy()) {
       type = FLOAT;
    } else if (allocTy->isDoubleTy()) {
       type = DOUBLE;
    } else if (allocTy->isPointerTy()) {
       type = PTR;
    } else
        errs() << "ALLOC TYPE: BYTES (NO TYPE SET)\n";
    errs() << "\n";
    
    
    std::string fname = "none.c";
    int line = 0;
    
    if (isMain) {
        std::swap(newI, oldI);
    }
    
    // Attempt to grab dbg info from llvm.dbg.declare and set it to all insts created when duplicating allocs
    //for( auto S = oldI->user_begin(), E = oldI->user_end(); S !=E; S++) {
    //    S->dump();
    //}
     for (auto BBs = newI->getParent()->getParent()->begin(); BBs != newI->getParent()->getParent()->end(); BBs++) {
         for (auto iter = BBs->begin(); iter != BBs->end(); iter++) {
         //for (BasicBlock::iterator iter(oldI); iter != oldI->getParent()->end(); iter++) {
            if (const DbgDeclareInst* DbgDeclare = dyn_cast<DbgDeclareInst>(iter)) {
                if (DbgDeclare->getAddress() == newI) { 
                    auto tmp = DIVariable(DbgDeclare->getVariable()).getFile();
                    fname = tmp.getDirectory().str() + "/" + tmp.getFilename().str();
                    line = DIVariable(DbgDeclare->getVariable()).getLineNumber();
                    break;
                }
            }
            else if (const DbgValueInst* DbgValue = dyn_cast<DbgValueInst>(iter)) {
                if (DbgValue->getValue() == newI) {
                    auto tmp = DIVariable(DbgValue->getVariable()).getFile();
                    fname = tmp.getDirectory().str() + "/" + tmp.getFilename().str();
                    line = DIVariable(DbgValue->getVariable()).getLineNumber();
                    break;
                }
            }
        }

    }
    
    createFileNameGlobal(fname);
    // Transforms a gold and faulty alloc:
    //      %A.addr = alloc ; gold
    //      %A.addr_f = alloc ; faulty
    //
    // Into:
    //      %A.addr = alloc ; gold
    //      %A.addr_f = alloc ; faulty
    //      %bitGold = bitcast %A.addr
    //      %bitFaulty = bitcast %A.addr_f
    //      __LOG_AND_RELATE_ALLOCA(...)


    AllocaInst* aI = dyn_cast<AllocaInst>(oldI);
    std::vector<Value*> args(7);
    //std::vector<Value*> args(4);
    unsigned long size;
    /*
        args[0] = faulty Ptr
        args[1] = orig ptr
        args[2] = num elements
        args[3] = elements size
    */
    args[0] = new BitCastInst(newI, Type::getInt8PtrTy(getGlobalContext()),
                "alloc_ptrCpy", Inext);
    args[1]= new BitCastInst(aI, Type::getInt8PtrTy(getGlobalContext()),
                "alloc_ptrOrig", Inext); // pointer
    if (aI->getArraySize()->getType()->isIntegerTy(64)) {
       args[2] =  aI->getArraySize();
    }
    else {
        args[2] = new ZExtInst(aI->getArraySize(), i64Ty, "nelm", Inext); // num elements
    }
    size = Layout->getTypeAllocSize(aI->getAllocatedType()); // element size
    args[3] = ConstantInt::get(i64Ty, size); // element size
    args[4] = ConstantInt::get(i32Ty, type);
    args[5] = gvar_fname;
    args[6] = ConstantInt::get(i32Ty, line);
    
    CallInst::Create(logAlloca, args, "log", Inext);
    if (isMain) {
        std::swap(newI, oldI);
    }
    return true;
}

void SDCProp::createFileNameGlobal(std::string fname)
{
    if (gvar_fname == NULL || (dyn_cast<ConstantDataArray>(fname_data)->getAsString().str().substr(0,6) == "none.c" && fname != "none.c")) {
        auto gvar = new GlobalVariable(*M, ArrayType::get(Type::getInt8Ty(getGlobalContext()), fname.size()+1), true, GlobalValue::PrivateLinkage, 0, ".fName");
        gvar->setAlignment(1);
        fname_data = ConstantDataArray::getString(M->getContext(), fname, true);

        std::vector<Constant*> gepArgs;
        gepArgs.push_back( ConstantInt::get(i32Ty, 0) );
        gepArgs.push_back( ConstantInt::get(i32Ty, 0) );
        gvar_fname = ConstantExpr::getGetElementPtr(gvar, gepArgs);

        gvar->setInitializer(fname_data);
    }
}

void SDCProp::handleMalloc(Instruction* newI, Instruction* oldI, Instruction* Inext, std::string name)
{
    int type = UNKNOWN;
    Type* allocTy = NULL;
    if (isa<BitCastInst>(Inext) && (!Inext->getType()->isVoidTy())) {
        allocTy = Inext->getType()->getContainedType(0);
        if (allocTy != NULL && allocTy->isArrayTy()) {
            allocTy = allocTy->getContainedType(0);
        }
    }
    
    if (allocTy == NULL) {
        type = UNKNOWN;
    } else if (allocTy->isIntegerTy()) {
       if (allocTy->isIntegerTy(8))
           type = INT8;
       else if (allocTy->isIntegerTy(16))
           type = INT16;
       else if (allocTy->isIntegerTy(32))
           type = INT32;
       else if (allocTy->isIntegerTy(64))
           type = INT64;
    } else if (allocTy->isFloatTy()) {
       type = FLOAT;
    } else if (allocTy->isDoubleTy()) {
       type = DOUBLE;
    } else if (allocTy->isPointerTy()) {
       type = PTR;
    } else
        errs() << "BYTES\n";
    
    std::string fname = "none.c";
    int line = 0;
    MDNode* N;
    std::vector<Value*> args(6);

    if (isMain) {
        N = newI->getMetadata("dbg");
        std::swap(newI, oldI);
    }
    else {
        flipitInsts.push_back(newI);
        N = oldI->getMetadata("dbg");
    }
    if (N != NULL) {
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR <= 6
        DILocation Loc(N);
        fname = Loc.getDirectory().str() + "/" + Loc.getFilename().str();
        line = Loc.getLineNumber();
            createFileNameGlobal(fname);
#elif LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 6
        DILocation* Loc = oldI->getDebugLoc();
        fname = Loc->getDirectory().str() +"/" + Loc->getFilename().str();
        line = Loc->getLineNumber();
            createFileNameGlobal(fname);
#endif
    }
////////////////////////////////////////////////////////
//


        int ll = -1;
        std::string fn = "NOPE.c";
        N = newI->getMetadata("dbg");
        if (N != NULL) {
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR <= 6
            DILocation Loc(N);
            fn = Loc.getDirectory().str() + "/" + Loc.getFilename().str();
            ll = Loc.getLineNumber();
                createFileNameGlobal(fn);
#elif LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 6
            DILocation* Loc = oldI->getDebugLoc();
            fn = Loc->getDirectory().str() +"/" + Loc->getFilename().str();
            ll = Loc->getLineNumber();
                createFileNameGlobal(fn);
#endif
        }
    
        N = oldI->getMetadata("dbg");
        if (N != NULL) {
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR <= 6
            DILocation Loc(N);
            fn = Loc.getDirectory().str() + "/" + Loc.getFilename().str();
            ll = Loc.getLineNumber();
                createFileNameGlobal(fn);
#elif LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 6
            DILocation* Loc = oldI->getDebugLoc();
            fn = Loc->getDirectory().str() +"/" + Loc->getFilename().str();
            ll = Loc->getLineNumber();
                createFileNameGlobal(fn);
#endif
        }

    ////////////////////////
    args[0] = newI;
    args[1] = oldI;
    if (name == "malloc" 
            || name.find("operator new") != std::string::npos) {
        args[2] = newI->getOperand(0); // malloc size
    }
    else { // calloc size * num
        args[2] = BinaryOperator::Create(Instruction::Mul, newI->getOperand(0),
                newI->getOperand(1), "", Inext);
    }
    //args[3] = gvar_fname;
    args[3] = ConstantInt::get(i32Ty, type); 
    args[4] = gvar_fname;//new LoadInst (gvar_fname, "fname", Inext);
    args[5] = ConstantInt::get(i32Ty, line);
    
    auto call = CallInst::Create(logMalloc, args, "", Inext);
    copyMetadata(call, newI);
    if (isMain) {
        std::swap(newI, oldI);
    }
}

void SDCProp::handleRealloc(Instruction* newI, Instruction* oldI, Instruction* Inext, std::string name)
{
    int type = UNKNOWN;
    Type* allocTy = NULL;
    if (isa<BitCastInst>(Inext) && (!Inext->getType()->isVoidTy())) {
        allocTy = Inext->getType()->getContainedType(0);
        if (allocTy != NULL && allocTy->isArrayTy()) {
            allocTy = allocTy->getContainedType(0);
        }
    }
    
    if (allocTy == NULL) {
        type = UNKNOWN;
    } else if (allocTy->isIntegerTy()) {
       if (allocTy->isIntegerTy(8))
           type = INT8;
       else if (allocTy->isIntegerTy(16))
           type = INT16;
       else if (allocTy->isIntegerTy(32))
           type = INT32;
       else if (allocTy->isIntegerTy(64))
           type = INT64;
    } else if (allocTy->isFloatTy()) {
       type = FLOAT;
    } else if (allocTy->isDoubleTy()) {
       type = DOUBLE;
    } else if (allocTy->isPointerTy()) {
       type = PTR;
    } else
        errs() << "BYTES\n";
    
    std::string fname = "none.c";
    int line = 0;
    MDNode* N;
    std::vector<Value*> args(8);

    if (isMain) {
        std::swap(newI, oldI);
    }
    flipitInsts.push_back(newI);
    N = oldI->getMetadata("dbg");
    if (N != NULL) {
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR <= 6
        DILocation Loc(N);
        fname = Loc.getDirectory().str() + "/" + Loc.getFilename().str();
        line = Loc.getLineNumber();
            createFileNameGlobal(fname);
#elif LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 6
        DILocation* Loc = oldI->getDebugLoc();
        fname = Loc->getDirectory().str() +"/" + Loc->getFilename().str();
        line = Loc->getLineNumber();
            createFileNameGlobal(fname);
#endif
    }

    args[0] = newI;
    args[1] = oldI;
    args[2] = newI->getOperand(0); // old ptr
    args[3] = oldI->getOperand(0);
    args[4] = newI->getOperand(1); // size
    args[5] = ConstantInt::get(i32Ty, type); 
    args[6] = gvar_fname;//new LoadInst (gvar_fname, "fname", Inext);
    args[7] = ConstantInt::get(i32Ty, line);
    
    auto call = CallInst::Create(logRealloc, args, "", Inext);
    copyMetadata(call, newI);
    if (isMain) {
        std::swap(newI, oldI);
    }
}

bool SDCProp::instrumentCall(Instruction* newI, Instruction* oldI, Instruction* Inext, std::map<Value*, Value*>&replace, std::vector<Instruction*>& toErase)
{
    // newI is the faulty call instruction
    // oldI is the original instruction
    // if we are inside the main function the roles are reversed

    // We have 3 options:
    // 1. Function is executed by gold and faulty code at different times
    //      e.g.  sqrt, pow, free, cos
    // 2. Function is only called in the gold code
    //      e.g.    MPI_Init
    // 3. Function is to be replaced by a GG_F version with the argument list
    //    extended to pass the faulty arguments as well as the gold ones
    //      e.g. User defined function in same file or another file
    //

    std::string fName = "";
    auto cNew = dyn_cast<CallInst>(newI);
    auto cOld = dyn_cast<CallInst>(oldI);
    
    std::string fname = "none.c";
    int line = 0;
    MDNode* N;
    N = oldI->getMetadata("dbg");
    if (N != NULL) {
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR <= 6
        DILocation Loc(N);
        //fname = Loc.getFilename();
        fname = Loc.getDirectory().str() + "/" + Loc.getFilename().str();
        line = Loc.getLineNumber();
            createFileNameGlobal(fname);
#elif LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 6
        DILocation* Loc = oldI->getDebugLoc();
        //fnpame = Loc->getFilename();
        fname = Loc->getDirectory().str() + "/" + Loc->getFilename().str();
        line = Loc->getLineNumber();
            createFileNameGlobal(fname);
#endif
    }

    // check if function pointer or not
    if (cNew->getCalledFunction() != NULL)
        fName = demangle(cNew->getCalledFunction()->getName().str());

    if (fName.find("malloc") != std::string::npos
            || fName.find("calloc") != std::string::npos
            || fName.find("operator new") != std::string::npos) {
        handleMalloc(newI, oldI, Inext, fName);
    }
    else if (fName.find("realloc") != std::string::npos) {
        handleRealloc(newI, oldI, Inext, fName);
    }

    // Case 1: called function is to be executed by gold and fault code
    else if (fName.find("free") != std::string::npos
            || fName.find("llvm.dbg") != std::string::npos
            || fName.find("llvm.") != std::string::npos
            || fName == ""
        || std::find(dupFuncs.begin(), dupFuncs.end(), fName) != dupFuncs.end()) {
 
        //log possible stack alloc from returned pointer
        if(newI->getType()->isPointerTy())
        {
            auto tmp = newI->getType()->getContainedType(0);

            //std::vector<Value*> args(4);
            std::vector<Value*> args(7);
            args[0] = new BitCastInst(newI, Type::getInt8PtrTy(getGlobalContext()),"fRet", Inext);
            args[1]= new BitCastInst(oldI, Type::getInt8PtrTy(getGlobalContext()), "gRet", Inext); // pointer
            args[2] = ConstantInt::get(i64Ty, 1); // num elements
            if (tmp->isSized())
                args[3] = ConstantInt::get(i64Ty, Layout->getTypeStoreSize(tmp));
            else {
                args[3] = ConstantInt::get(i64Ty, 4);
            }
                
            args[4] = ConstantInt::get(i32Ty, PTR);
            args[5] = gvar_fname;
            args[6] = ConstantInt::get(i32Ty, line);
            CallInst::Create(logAlloca, args, "log", Inext);
            nAllocas++;
       // }
        }
        return true;
    //Ty->isSized()
    }
    // Case 2 and 3:
    else {
        if (isMain) {
            std::swap(newI, oldI);
        }
        cNew = dyn_cast<CallInst>(newI);
        cOld = dyn_cast<CallInst>(oldI);
        if (std::find(singleFuncs.begin(), singleFuncs.end(), fName) == singleFuncs.end()) {

            // Case 3: function is to be called by both the gold and faulty code as a single call

            // get a handle to the faulty function
            // TODO: does demangling a GG_F function affect what I expect to find?
            Function* func = NULL;
            for (auto f : faultyFuncs) {
                if (f->getName().find(fName +"GG_F") != std::string::npos
                    || f->getName().find(cNew->getCalledFunction()->getName().str()+"GG_F") != std::string::npos) {
                    func = f; break;
                }
            }

            // if we don't have a faulty prototype create one
            if (func == NULL) {
                //errs() << "Making faulty proto type for func :" << fName << "\n";
                    func = createProtoType(cNew->getCalledFunction(), "GG_F",/*arg appears 2x*/ 2);
                faultyFuncs.push_back(func);
            }

            // create the new argument list (gold, ..., faulty, ...)
            std::vector<Value*> args;
            for (unsigned i=0; i<cOld->getNumArgOperands(); i++) {
                args.push_back(cOld->getArgOperand(i));
            }
            for (unsigned i=0; i<cNew->getNumArgOperands(); i++) {
                args.push_back(cNew->getArgOperand(i));
            }

            //Form call and retrive the return value
            CallInst* call;
            if (oldI->getType()->isVoidTy())
               call = CallInst::Create(func, args, "", oldI);
            else {
                // return value
                call = CallInst::Create(func, args, "instCall", oldI);
                auto newIRet = ExtractValueInst::Create(call, {0}, "extract", Inext);
                auto oldIRet = ExtractValueInst::Create(call, {1}, "extract", Inext);
                newI->replaceAllUsesWith(newIRet);
                oldI->replaceAllUsesWith(oldIRet);
            }

            // enforce the calling conventions are honored
            call->setCallingConv(cNew->getCallingConv());
            toErase.push_back(newI);
            toErase.push_back(oldI); 
        }
        else /* single called function */{
            toErase.push_back(newI);// don't need the faulty function call

            // x = call @foo(args)
            // x_f = bitcast x to same type ( if x.type != void)
            // __LOG_ALLOCA() (only if x.type is PtrTy)
            if (!oldI->getType()->isVoidTy()) {
                // bitcast simulates the fault function call for a return value
                auto ret = new BitCastInst(oldI, oldI->getType(), "tmp", Inext);
                newI->replaceAllUsesWith(ret);

                // Log the alloc 
                if (oldI->getType()->isPointerTy()) {
                    //std::vector<Value*> args(4);
                    std::vector<Value*> args(7);
                    auto tmp = oldI->getType()->getContainedType(0);
                if(tmp->isSized()) {//errs() << "arg0\n";
                    args[0] = new BitCastInst(ret, Type::getInt8PtrTy(getGlobalContext()),"fRet", Inext);
                    args[1]= new BitCastInst(oldI, Type::getInt8PtrTy(getGlobalContext()), "gRet", Inext); // pointer
                    args[2] = ConstantInt::get(i64Ty, 1); // num elements
                    args[3] = ConstantInt::get(i64Ty, Layout->getTypeStoreSize(tmp));
                    args[4] = ConstantInt::get(i32Ty, PTR);
                    args[5] = gvar_fname;
                    args[6] = ConstantInt::get(i32Ty, line);
                    CallInst::Create(logAlloca, args, "log", Inext);
                    nAllocas++;
}
                }
            }
            /* DEBUG */ //newI->getParent()->dump();
        }
        if (isMain) {
            std::swap(newI, oldI);
        }
    }
    
    return true;
}

bool SDCProp::instrumentCompare(Instruction* newI, Instruction* oldI, Instruction* Inext)
{
    // Output metadata about branch location
    MDNode* N = oldI->getMetadata("dbg");
    if (N != NULL) {
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR <= 6
        DILocation Loc(N);
        errs() << Loc.getFilename() << ":" << Loc.getLineNumber() << "\n";
#elif LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 6
        DILocation* Loc = oldI->getDebugLoc();
        errs() << Loc->getFilename() << ":" << Loc->getLine() << "\n";
#endif
    }
    else { // function name
        errs() << demangle(oldI->getParent()->getParent()->getName().str()) << "\n";
    }

    if (isMain) {
        std::swap(newI, oldI);
    }
    /*
        arg[0] = origCmp (i1)
        arg[1] = copyCmp (i1)
        arg[2] = BB_ID
    */
    std::vector<Value*> args(3);
    args[0] = new ZExtInst(newI, Type::getInt32Ty(getGlobalContext()), "zXCmpCopy", Inext);
    args[1] = new ZExtInst(oldI, Type::getInt32Ty(getGlobalContext()), "zXCmpOrig", Inext);
    args[2] = ConstantInt::get(i32Ty, BB_ID);
    BB_ID++;
    CallInst::Create(checkCmp, args, "", Inext);

    if (isMain) {
        std::swap(newI, oldI);
    }
    return true;
}

bool SDCProp::copyMetadata(Instruction* New, Instruction* Old)
{
    MDNode* N = Old->getMetadata("dbg");
    if (N != NULL) {
        New->setMetadata("dbg", N);
        //DILocation Loc(New->getMetadata("dbg"));
    }
    return N != NULL;
}

Instruction* SDCProp::convertType(Value* from, Type* type, Instruction* insertBefore)
{
    //NOTE: Only converts to and from i64Ty
    auto fType = from->getType();
    if (fType->isIntegerTy(64)) {
        if(type->isIntegerTy()) {
            int nBytes = type->getIntegerBitWidth()/8;
            if (nBytes != 8) {
                return new TruncInst(from, type, "argTrunc", insertBefore);    
            }
            else {
                return dyn_cast<Instruction>(from);
            }
        }
        else if (type->isFloatTy()) {
            auto toFloat = new TruncInst(from, Type::getInt32Ty(getGlobalContext()),\
                    "argTrunc2f32", insertBefore);
            return new BitCastInst(toFloat, type, "argBitCastf32", insertBefore);
        }
        else if (type->isDoubleTy()) {
            return new BitCastInst(from, type, "argBitCast", insertBefore);
        }
        else if (type->isPointerTy()) {
            return new IntToPtrInst(from, type, "argI2P", insertBefore);
        }
    }
    
    /* Converting to i64Ty */
    else if(fType->isIntegerTy()) {
        int nBytes = fType->getIntegerBitWidth()/8;
        if (nBytes != 8) {
            return new ZExtInst(from, type, "zext", insertBefore);    
        }
    }
    else if (fType->isFloatTy()) {
        auto toFloat = new BitCastInst(from, Type::getInt32Ty(getGlobalContext()),\
                "f32Toi32", insertBefore);
        return new ZExtInst(toFloat, type, "i32Toi64", insertBefore);
    }
    else if (fType->isDoubleTy()) {
        return new BitCastInst(from, type, "dblToi64", insertBefore);
    }
    else if (fType->isPointerTy()) {
        return new PtrToIntInst(from, type, "P2I", insertBefore);
    }
    return NULL;
}

void SDCProp::removeAllocas(int num, Instruction* Inext)
{
    std::vector<Value*> args (1);
    args[0] = ConstantInt::get(i32Ty, num);
    CallInst::Create(rmAlloca, args, "", Inext);
}

void SDCProp::handleSwitch(Instruction* newI, Instruction* oldI, std::vector<Instruction*>& toErase)
{
    // Output metadata about switch location
    errs() << "BB #"<< BB_ID <<" (" << newI->getParent()->getName().str() <<"): ";
    MDNode* N = newI->getMetadata("dbg");
    //errs() << "grabbing dbg info\n";
    if (N != NULL) {
#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR <= 6
        DILocation Loc(N);
        errs() << Loc.getFilename() << ":" << Loc.getLineNumber() << "\n";
#elif LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR > 6
        DILocation* Loc = oldI->getDebugLoc();
        errs() << Loc->getFilename() << ":" << Loc->getLine() << "\n";
#endif
    }
    else { // function name
        errs() << "ELSE\n";
        errs() << oldI->getParent()->getParent()->getName().str() << "\n";
    }

    // work around for main()
    if (isMain) {
        std::swap(newI, oldI);
    }
    auto newSwitch = dyn_cast<SwitchInst>(newI);
    auto oldSwitch = dyn_cast<SwitchInst>(oldI);
    
    // check for control flow divergence
    /*
        arg[0] = origCmp (i1)
        arg[1] = copyCmp (i1)
        arg[2] = BB_ID
    */
    std::vector<Value*> args(3);
    //args[0] = newSwitch->getCondition();//new ZExtInst(newSwitch, Type::getInt32Ty(getGlobalContext()), "zXCmpCopy", newSwitch);
    if (newSwitch->getCondition()->getType()->isIntegerTy(32)) {
        args[0] = newSwitch->getCondition();//new ZExtInst(newSwitch, Type::getInt32Ty(getGlobalContext()), "zXCmpCopy", newSwitch);
        args[1] = oldSwitch->getCondition();//new ZExtInst(oldSwitch, Type::getInt32Ty(getGlobalContext()), "zXCmpOrig", newSwitch);
    } else {
        args[0] = new ZExtInst(newSwitch, Type::getInt32Ty(getGlobalContext()), "zXCmpCopy", newSwitch);
        //args[1] = oldSwitch->getCondition();//new ZExtInst(oldSwitch, Type::getInt32Ty(getGlobalContext()), "zXCmpOrig", newSwitch);
        args[1] = new ZExtInst(oldSwitch, Type::getInt32Ty(getGlobalContext()), "zXCmpOrig", oldSwitch);
    }
    args[2] = ConstantInt::get(i32Ty, BB_ID);
    BB_ID++;
    
    /*
     * assert(newSwitch->getParent() != NULL && "NEWS BB IS NULL\n");
   errs() << "---\n" << oldSwitch->getParent() << "\n";
    oldSwitch->getParent()->dump();
    errs() << "___\n";
    assert(oldSwitch->getParent()->getParent() != NULL && "NEWS FUNC IS NULL\n");
    errs() << "---\n" << oldSwitch->getParent()->getParent() << "\n";
    oldSwitch->getParent()->getParent()->dump();
    errs() << "___\n";
    */
    CallInst::Create(checkCmp, args, "", isMain ? oldSwitch : newSwitch);

    // update labels for correct branching
    if (!isMain) {
        unsigned i=0;
        for (auto c : newSwitch->cases()) {
            oldSwitch->setSuccessor(i, c.getCaseSuccessor());
            i++;
        }
        newI->getParent()->getInstList().insert(newI, oldI);
        toErase.push_back(oldI);
    }
    else {
        oldI->getParent()->getInstList().insert(oldI, newI);
        toErase.push_back(newI);
        std::swap(newI, oldI);
    }
}

bool SDCProp::handleInvoke(Instruction* dup, Instruction* orig, Instruction* Inext, std::vector<Instruction*>& toErase)
{
    std::string fName = "";
    auto iDup = dyn_cast<InvokeInst>(dup);
    auto iOrig = dyn_cast<InvokeInst>(orig);
    bool ret = false;
    // check if function pointer or not
    if (iOrig->getCalledFunction() != NULL)
        fName = demangle(iOrig->getCalledFunction()->getName().str());
    
    if (isMain) {
        std::swap(dup, orig);
    }
    iDup = dyn_cast<InvokeInst>(dup);
    iOrig = dyn_cast<InvokeInst>(orig);

    // Case 1: called function is to be executed by gold and fault code
    if (std::find(dupFuncs.begin(), dupFuncs.end(), fName) != dupFuncs.end()) {
 
        //log possible stack alloc from returned pointer
        if(iOrig->getType()->isPointerTy())
        {
            auto tmp = iOrig->getType()->getContainedType(0);

            std::vector<Value*> args(4);
            args[0] = new BitCastInst(dup, Type::getInt8PtrTy(getGlobalContext()),"fRet", Inext);
            args[1]= new BitCastInst(orig, Type::getInt8PtrTy(getGlobalContext()), "gRet", Inext); // pointer
            args[2] = ConstantInt::get(i64Ty, 1); // num elements
            args[3] = ConstantInt::get(i64Ty, Layout->getTypeStoreSize(tmp));
            CallInst::Create(logAlloca, args, "log", Inext);
            nAllocas++;
        }
            iDup->setNormalDest(iOrig->getNormalDest());
            iDup->setUnwindDest(iOrig->getUnwindDest());
        /*if (isMain) {
            iOrig->setNormalDest(iDup->getNormalDest());
            iOrig->setUnwindDest(iDup->getUnwindDest());
        }
        else {
        }*/
        ret = true;
    }
    // Case 2 and 3:
    else {
        if (std::find(singleFuncs.begin(), singleFuncs.end(), fName) == singleFuncs.end()) {

            // Case 3: function is to be called by both the gold and faulty code as a single call

            // get a handle to the faulty function
            // TODO: does demangling a GG_F function affect what I expect to find?
            Function* func = NULL;
            for (auto f : faultyFuncs) {
                if (f->getName().find(fName +"GG_F") != std::string::npos) {
                    func = f; break;
                }
            }

            // if we don't have a faulty prototype create one
            if (func == NULL) {
                func = createProtoType(iDup->getCalledFunction(), "GG_F",/*arg appears 2x*/ 2);
                faultyFuncs.push_back(func);
            }

            // create the new argument list (gold, ..., faulty, ...)
            std::vector<Value*> args;
            for (unsigned i=0; i<iOrig->getNumArgOperands(); i++) {
                args.push_back(iOrig->getArgOperand(i));
            }
            for (unsigned i=0; i<iDup->getNumArgOperands(); i++) {
                args.push_back(iDup->getArgOperand(i));
            }

            //Form call and retrive the return value
            InvokeInst* invoke;
            if (orig->getType()->isVoidTy()) {
                invoke = InvokeInst::Create(func, iOrig->getNormalDest(),
                                iOrig->getUnwindDest(), args, "", orig); // is orig inserted?
            }
            else {
                // return value
                invoke = InvokeInst::Create(func, iOrig->getNormalDest(),
                                iOrig->getUnwindDest(), args, "instInvoke", orig); // is orig inserted?
                auto dupRet = ExtractValueInst::Create(invoke, {0}, "extract", Inext);
                auto origRet = ExtractValueInst::Create(invoke, {1}, "extract", Inext);
                dup->replaceAllUsesWith(dupRet);
                orig->replaceAllUsesWith(origRet);
            }

            // enforce the calling conventions are honored
            invoke->setCallingConv(iDup->getCallingConv());
            toErase.push_back(dup);
            toErase.push_back(orig);
            ret = true;
        }
        else /* single called function */{
            toErase.push_back(dup);// don't need the faulty function call

            // x = call @foo(args)
            // x_f = bitcast x to same type ( if x.type != void)
            // __LOG_ALLOCA() (only if x.type is PtrTy)

            if (!orig->getType()->isVoidTy()) {
                // bitcast simulates the fault function call for a return value
                auto ret_f = new BitCastInst(orig, orig->getType(), "tmp", Inext);
                dup->replaceAllUsesWith(ret_f);

                // Log the alloc 
                if (orig->getType()->isPointerTy()) {
                    std::vector<Value*> args(4);
                    auto tmp = orig->getType()->getContainedType(0);
                    args[0] = new BitCastInst(ret_f, Type::getInt8PtrTy(getGlobalContext()),"fRet", Inext);
                    args[1]= new BitCastInst(orig, Type::getInt8PtrTy(getGlobalContext()), "gRet", Inext); // pointer
                    args[2] = ConstantInt::get(i64Ty, 1); // num elements
                    args[3] = ConstantInt::get(i64Ty, Layout->getTypeStoreSize(tmp));
                    CallInst::Create(logAlloca, args, "log", Inext);
                    InvokeInst::Create(logAlloca, iOrig->getNormalDest(),
                                iOrig->getUnwindDest(), args, "log", orig); // is orig inserted?
                    nAllocas++;
                    ret = true;
                }
            }
        }
    }
    
    if (isMain) {
        std::swap(dup, orig);
    }
    return ret;
}
