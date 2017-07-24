%Mem8Type = type [4294967296 x i8]
%Mem16Type = type [2147483648 x i16]
;%Mem32Type = type [1024 x i32]
;@MEMORY = global [1024 x i32] zeroinitializer
%Mem32Type = type [1073741824 x i32]
@MEMORY = global [1073741824 x i32] zeroinitializer
; @MEMORY_PTR = global i32* getelementptr i32* @MEMORY, i32 0

define i32 @__ldl_mmu(i32 %addr, i32 %mmu_idx)
alwaysinline
{
	; assume addr is aligned
	; assume native endiannes
	%idx = udiv i32 %addr, 4
	%ptr = getelementptr %Mem32Type* @MEMORY, i32 0, i32 %idx
	%val = load i32* %ptr
	ret i32 %val
}

define void @__stl_mmu(i32 %addr, i32 %val, i32 %mmu_idx)
alwaysinline noimplicitfloat nounwind
{
	; assume addr is aligned
	%idx = udiv i32 %addr, 4
	%ptr = getelementptr %Mem32Type* @MEMORY, i32 0, i32 %idx
	store i32 %val, i32* %ptr
	ret void
}

define zeroext i16 @__ldw_mmu(i32 %addr, i32 %mmu_idx)
alwaysinline noimplicitfloat nounwind
{
	; assume addr is aligned
	%idx = udiv i32 %addr, 4
	%ptr = getelementptr i16* bitcast (%Mem32Type* @MEMORY to i16*), i32 %idx
	%val = load i16* %ptr
	ret i16 %val
}

define void @__stw_mmu(i32 %addr, i16 zeroext %val, i32 %mmu_idx)
alwaysinline noimplicitfloat nounwind
{
	; assume addr is aligned
	%idx = udiv i32 %addr, 2
	%ptr = getelementptr i16* bitcast (%Mem32Type* @MEMORY to i16*), i32 %addr
	store i16 %val, i16* %ptr
	ret void
}

define zeroext i8 @__ldb_mmu(i32 %addr, i32 %mmu_idx)
alwaysinline noimplicitfloat nounwind
{
	%ptr = getelementptr i8* bitcast (%Mem32Type* @MEMORY to i8*), i32 %addr
	%val = load i8* %ptr
	ret i8 %val
}

define void @__stb_mmu(i32 %addr, i8 zeroext %val, i32 %mmu_idx)
alwaysinline noimplicitfloat nounwind
{
	%ptr = getelementptr i8* bitcast (%Mem32Type* @MEMORY to i8*), i32 %addr
	store i8 %val, i8* %ptr
	ret void
}
