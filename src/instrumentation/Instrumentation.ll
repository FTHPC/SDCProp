target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
declare  void @__cxx_global_var_init() section ".text.startup"
declare void @__LOG_AND_RELATE_GLOBAL(i8* %fptr, i8* %gptr, i64 %size) #1
declare void @__LOG_MALLOCS_IN_CALL() #3
declare void @__RELATE_MALLOCS_IN_CALL(i8 signext %type) #1
declare void @__LOG_CMD_ARGS(i32 %argc, i8** %argv) #1
declare void @__REMOVE_ALLOCA(i32 %num) #1
declare i32 @__LOG_ALLOCA(i8* %ptr, i64 %num, i64 %size) #1
declare void @__DUMMY_FREE(i8* %ptr) #3
declare i64 @__GET_FAULTY_VALUE(i64 %gaddr, i64 %faddr, i64 %gvalue) #1
declare i32 @__LOG_AND_RELATE_ALLOCA(i8* %fPtr, i8* %gPtr, i64 %num, i64 %size, i32 %type, i8* %fname, i32 %line) #1
declare void @__LOG_AND_RELATE_MALLOC(i8* %fPtr, i8* %gPtr, i64 %size, i32 %type, i8* %fname, i32 %line) #1
declare void @__LOG_AND_RELATE_REALLOC(i8* %fPtr, i8* %gPtr, i8* %old_fptr, i8* %old_gptr, i64 %size, i32 %type, i8* %fname, i32 %line) #1
declare void @__CHECK_CMP(i32 %faulty, i32 %gold, i32 %BB_ID) #1
declare void @__START_FUNCTION() #1
declare void @__STORE_INT(i64 %fptr, i64 %fvalue, i64 %gptr, i64 %gvalue, i32 %nBytes, i64 %mask) #1
declare void @__STORE_FLOAT(i64 %fptr, float %fvalue, i64 %gptr, float %gvalue, float %mask) #1
declare void @__STORE_DOUBLE(i64 %fptr, double %fvalue, i64 %gptr, double %gvalue, double %mask) #1
declare void @__STORE_PTR(i64 %fptr, i64 %fvalue, i64 %gptr, i64 %gvalue, i8* %mask) #1
declare void @__STORE_VECTOR(i64 %fptr, i64 %gptr, i32 %nElms, i32 %type, double %mask) #1
declare i64 @__LOAD_INT(i64 %fptr, i64 %gptr, i64 %gvalue, i64 %mask, i32 %nBytes) #1
declare float @__LOAD_FLOAT(i64 %fptr, i64 %gptr, float %gvalue, float %mask) #1
declare double @__LOAD_DOUBLE(i64 %fptr, i64 %gptr, double %gvalue, double %mask) #1
declare i64 @__LOAD_PTR(i64 %fptr, i64 %gptr, i64 %gvalue, i8* %mask) #1
declare double @__LOAD_VECTOR_ELEMENT(i64 %fptr, i64 %gptr, i32 %elm, i32 %type, double %mask) #1
declare  void @_GLOBAL__sub_I_Instrumentation.cpp() section ".text.startup"
attributes #1 = { uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #10 = { nobuiltin nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #11 = { noreturn "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #12 = { nobuiltin "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #13 = { inlinehint uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #14 = { noreturn nounwind }
attributes #15 = { builtin }
attributes #16 = { nounwind readonly }
attributes #17 = { nounwind readnone }
attributes #18 = { noreturn }
