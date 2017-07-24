%Mem8Type = type [4294967296 x i8]
%Mem16Type = type [2147483648 x i16]
%Mem32Type = type [1073741824 x i32]

; assume endian little :-(
; memory small, useful for testing
;%Mem8Type = type [4096 x i8]
;%Mem16Type = type [2048 x i16]
;%Mem32Type = type [1024 x i32]
;target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
;target triple = "x86_64-unknown-linux-gnu"

@MEMORY = global %Mem8Type zeroinitializer


define i32 @__ldl_mmu(i32 %addr, i32 %mmu_idx)
alwaysinline
{
	%addr0 = add i32 %addr, 0
	%addr1 = add i32 %addr, 1
	%addr2 = add i32 %addr, 2
	%addr3 = add i32 %addr, 3

	%ptr0 = getelementptr %Mem8Type* @MEMORY, i32 0, i32 %addr0
	%ptr1 = getelementptr %Mem8Type* @MEMORY, i32 0, i32 %addr1
	%ptr2 = getelementptr %Mem8Type* @MEMORY, i32 0, i32 %addr2
	%ptr3 = getelementptr %Mem8Type* @MEMORY, i32 0, i32 %addr3

	%val0 = load i8* %ptr0
	%val1 = load i8* %ptr1
	%val2 = load i8* %ptr2
	%val3 = load i8* %ptr3

	%valE0 = zext i8 %val0 to i32
	%valE1 = zext i8 %val1 to i32
	%valE2 = zext i8 %val2 to i32
	%valE3 = zext i8 %val3 to i32

	%valS0 = shl i32 %valE0, 0
	%valS1 = shl i32 %valE1, 8
	%valS2 = shl i32 %valE2, 16
	%valS3 = shl i32 %valE3, 24

	%valH = or i32 %valS0, %valS1
	%valL = or i32 %valS2, %valS3

	%val = or i32 %valH, %valL

	ret i32 %val
}

define void @__stl_mmu(i32 %addr, i32 %val, i32 %mmu_idx)
alwaysinline noimplicitfloat nounwind
{
	%addr0 = add i32 %addr, 0
	%addr1 = add i32 %addr, 1
	%addr2 = add i32 %addr, 2
	%addr3 = add i32 %addr, 3

	%ptr0 = getelementptr %Mem8Type* @MEMORY, i32 0, i32 %addr0
	%ptr1 = getelementptr %Mem8Type* @MEMORY, i32 0, i32 %addr1
	%ptr2 = getelementptr %Mem8Type* @MEMORY, i32 0, i32 %addr2
	%ptr3 = getelementptr %Mem8Type* @MEMORY, i32 0, i32 %addr3

	%val0 = lshr i32 %val, 0
	%val1 = lshr i32 %val, 8
	%val2 = lshr i32 %val, 16
	%val3 = lshr i32 %val, 24

	%valT0 = trunc i32 %val0 to i8
	%valT1 = trunc i32 %val1 to i8
	%valT2 = trunc i32 %val2 to i8
	%valT3 = trunc i32 %val3 to i8

	store i8 %valT0, i8* %ptr0
	store i8 %valT1, i8* %ptr1
	store i8 %valT2, i8* %ptr2
	store i8 %valT3, i8* %ptr3

	ret void
}

define zeroext i16 @__ldw_mmu(i32 %addr, i32 %mmu_idx)
alwaysinline noimplicitfloat nounwind
{
	%addr0 = add i32 %addr, 0
	%addr1 = add i32 %addr, 1

	%ptr0 = getelementptr %Mem8Type* @MEMORY, i32 0, i32 %addr0
	%ptr1 = getelementptr %Mem8Type* @MEMORY, i32 0, i32 %addr1

	%val0 = load i8* %ptr0
	%val1 = load i8* %ptr1

	%valE0 = zext i8 %val0 to i16
	%valE1 = zext i8 %val1 to i16

	%valS0 = shl i16 %valE0, 0
	%valS1 = shl i16 %valE1, 8

	%val = or i16 %valS0, %valS1

	ret i16 %val
}

define void @__stw_mmu(i32 %addr, i16 zeroext %val, i32 %mmu_idx)
alwaysinline noimplicitfloat nounwind
{
	%addr0 = add i32 %addr, 0
	%addr1 = add i32 %addr, 1

	%ptr0 = getelementptr %Mem8Type* @MEMORY, i32 0, i32 %addr0
	%ptr1 = getelementptr %Mem8Type* @MEMORY, i32 0, i32 %addr1

	%val0 = lshr i16 %val, 0
	%val1 = lshr i16 %val, 8

	%valT0 = trunc i16 %val0 to i8
	%valT1 = trunc i16 %val1 to i8

	store i8 %valT0, i8* %ptr0
	store i8 %valT1, i8* %ptr1

	ret void
}

define zeroext i8 @__ldb_mmu(i32 %addr, i32 %mmu_idx)
alwaysinline noimplicitfloat nounwind
{
	%addr0 = add i32 %addr, 0

	%ptr0 = getelementptr %Mem8Type* @MEMORY, i32 0, i32 %addr0

	%val0 = load i8* %ptr0

	ret i8 %val0
}

define void @__stb_mmu(i32 %addr, i8 zeroext %val, i32 %mmu_idx)
alwaysinline noimplicitfloat nounwind
{
	%addr0 = add i32 %addr, 0

	%ptr0 = getelementptr %Mem8Type* @MEMORY, i32 0, i32 %addr0

	store i8 %val, i8* %ptr0

	ret void
}
