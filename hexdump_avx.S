.intel_syntax noprefix

.section .rodata
.p2align 5
hex_digits:
    .ascii "0123456789abcdef0123456789abcdef"
hex_digits_upper:
    .ascii "0123456789ABCDEF"
.p2align 4
reverse8_mask:
    .byte 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
    .byte 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
space_bytes:
    .fill 16, 1, 0x20
expand_mask:
    .byte 0x00, 0x01, 0x08, 0x02, 0x03, 0x08, 0x04, 0x05
    .byte 0x08, 0x06, 0x07, 0x08, 0x80, 0x80, 0x80, 0x80
byte_0f:
    .byte 0x0f
byte_80:
    .byte 0x80
byte_9f:
    .byte 0x9f
byte_ff:
    .byte 0xff
byte_dot:
    .byte 0x2e

.text
.globl hexdump_avx
.type hexdump_avx, @function
hexdump_avx:
    test rsi, rsi
    je .Ldone

    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 160

    mov r12, rdi
    mov r13, rsi
    xor ebx, ebx

.Lline:
    cmp rbx, r13
    jae .Lcleanup

    lea r14, [rsp]
    lea r15, [rsp + 112]

    mov BYTE PTR [r14 + 0], 0x5b
    mov BYTE PTR [r14 + 1], 0x30
    mov BYTE PTR [r14 + 2], 0x78
    mov BYTE PTR [r14 + 19], 0x5d
    mov BYTE PTR [r14 + 20], 0x5b
    mov BYTE PTR [r14 + 37], 0x3a
    mov BYTE PTR [r14 + 38], 0x20
    vmovdqa xmm0, XMMWORD PTR [rip + hex_digits_upper]
    vmovdqa xmm1, XMMWORD PTR [rip + reverse8_mask]
    vpbroadcastb xmm2, BYTE PTR [rip + byte_0f]
    vpxor xmm5, xmm5, xmm5

    movabs rax, 0x90e630f8e0
    add rax, rbx
    vmovq xmm3, rax
    vpshufb xmm3, xmm3, xmm1
    vpunpcklbw xmm3, xmm3, xmm5
    vpsrlw xmm4, xmm3, 4
    vpand xmm3, xmm3, xmm2
    vpand xmm4, xmm4, xmm2
    vpackuswb xmm3, xmm3, xmm3
    vpackuswb xmm4, xmm4, xmm4
    vpshufb xmm3, xmm0, xmm3
    vpshufb xmm4, xmm0, xmm4
    vpunpcklbw xmm3, xmm4, xmm3
    vmovdqu XMMWORD PTR [r14 + 3], xmm3

    vmovq xmm3, rbx
    vpshufb xmm3, xmm3, xmm1
    vpunpcklbw xmm3, xmm3, xmm5
    vpsrlw xmm4, xmm3, 4
    vpand xmm3, xmm3, xmm2
    vpand xmm4, xmm4, xmm2
    vpackuswb xmm3, xmm3, xmm3
    vpackuswb xmm4, xmm4, xmm4
    vpshufb xmm3, xmm0, xmm3
    vpshufb xmm4, xmm0, xmm4
    vpunpcklbw xmm3, xmm4, xmm3
    vmovdqu XMMWORD PTR [r14 + 21], xmm3

    mov rax, r13
    sub rax, rbx
    cmp rax, 16
    jb .Ltail

    lea r11, [r14 + 39]
    lea rsi, [rip + hex_digits]

    vmovdqa ymm0, YMMWORD PTR [rip + hex_digits]
    vpbroadcastb ymm1, BYTE PTR [rip + byte_0f]
    vpbroadcastb ymm2, BYTE PTR [rip + byte_80]
    vpbroadcastb ymm3, BYTE PTR [rip + byte_9f]
    vpbroadcastb ymm4, BYTE PTR [rip + byte_ff]
    vpbroadcastb ymm5, BYTE PTR [rip + byte_dot]
    vbroadcasti128 ymm6, XMMWORD PTR [r12 + rbx]
    vpsrlw ymm7, ymm6, 4
    vpand ymm8, ymm6, ymm1
    vpand ymm7, ymm7, ymm1
    vpshufb ymm8, ymm0, ymm8
    vpshufb ymm7, ymm0, ymm7
    vpunpcklbw ymm9, ymm7, ymm8
    vpunpckhbw ymm10, ymm7, ymm8
    vmovdqu XMMWORD PTR [r15 + 0], xmm9
    vmovdqu XMMWORD PTR [r15 + 16], xmm10

    vpxor ymm11, ymm6, ymm2
    vpcmpgtb ymm12, ymm11, ymm3
    vpcmpgtb ymm13, ymm4, ymm11
    vpand ymm12, ymm12, ymm13
    vpblendvb ymm14, ymm5, ymm6, ymm12

    vmovdqa xmm0, XMMWORD PTR [rip + space_bytes]
    vmovdqa xmm1, XMMWORD PTR [rip + expand_mask]

    vmovq xmm2, QWORD PTR [r15 + 0]
    vpunpcklqdq xmm2, xmm2, xmm0
    vpshufb xmm2, xmm2, xmm1
    vmovdqu XMMWORD PTR [r11 + 0], xmm2

    vmovq xmm2, QWORD PTR [r15 + 8]
    vpunpcklqdq xmm2, xmm2, xmm0
    vpshufb xmm2, xmm2, xmm1
    vmovdqu XMMWORD PTR [r11 + 12], xmm2

    vmovq xmm2, QWORD PTR [r15 + 16]
    vpunpcklqdq xmm2, xmm2, xmm0
    vpshufb xmm2, xmm2, xmm1
    vmovdqu XMMWORD PTR [r11 + 24], xmm2

    vmovq xmm2, QWORD PTR [r15 + 24]
    vpunpcklqdq xmm2, xmm2, xmm0
    vpshufb xmm2, xmm2, xmm1
    vmovdqu XMMWORD PTR [r11 + 36], xmm2

    mov BYTE PTR [r14 + 86], 0x20
    mov BYTE PTR [r14 + 87], 0x7c
    mov BYTE PTR [r14 + 88], 0x20
    vmovdqu XMMWORD PTR [r14 + 89], xmm14
    mov BYTE PTR [r14 + 105], 0x20
    mov BYTE PTR [r14 + 106], 0x5d
    mov BYTE PTR [r14 + 107], 0x0a

    mov r8d, 108
    add rbx, 16
    jmp .Lwrite

.Ltail:
    lea rsi, [rip + hex_digits]
    lea r8, [r14 + 39]
    mov r10, r13
    sub r10, rbx
    lea rdx, [r12 + rbx]
    xor r9d, r9d

.Ltail_hex:
    cmp r9, r10
    jae .Ltail_pad
    test r9, r9
    je .Ltail_byte
    mov BYTE PTR [r8], 0x20
    inc r8
.Ltail_byte:
    movzx ecx, BYTE PTR [rdx + r9]
    mov eax, ecx
    shr ecx, 4
    and eax, 0x0f
    mov cl, BYTE PTR [rsi + rcx]
    mov al, BYTE PTR [rsi + rax]
    mov BYTE PTR [r8], cl
    mov BYTE PTR [r8 + 1], al
    add r8, 2
    inc r9
    jmp .Ltail_hex

.Ltail_pad:
    lea r9, [r14 + 86]
.Ltail_pad_loop:
    cmp r8, r9
    jae .Ltail_ascii_prefix
    mov BYTE PTR [r8], 0x20
    inc r8
    jmp .Ltail_pad_loop

.Ltail_ascii_prefix:
    mov BYTE PTR [r14 + 86], 0x20
    mov BYTE PTR [r14 + 87], 0x7c
    mov BYTE PTR [r14 + 88], 0x20
    lea r8, [r14 + 89]
    xor r9d, r9d

.Ltail_ascii:
    cmp r9, r10
    jae .Ltail_suffix
    movzx eax, BYTE PTR [rdx + r9]
    cmp eax, 32
    jb .Ltail_dot
    cmp eax, 126
    ja .Ltail_dot
    mov BYTE PTR [r8 + r9], al
    inc r9
    jmp .Ltail_ascii

.Ltail_dot:
    mov BYTE PTR [r8 + r9], 0x2e
    inc r9
    jmp .Ltail_ascii

.Ltail_suffix:
    mov BYTE PTR [r8 + r10], 0x20
    mov BYTE PTR [r8 + r10 + 1], 0x5d
    mov BYTE PTR [r8 + r10 + 2], 0x0a
    lea r8, [r10 + 92]
    add rbx, r10

.Lwrite:
    xor r9d, r9d
.Lwrite_loop:
    mov eax, 1
    mov edi, 1
    lea rsi, [r14 + r9]
    mov rdx, r8
    sub rdx, r9
    syscall
    cmp rax, 0
    jle .Lcleanup
    add r9, rax
    cmp r9, r8
    jb .Lwrite_loop
    jmp .Lline

.Lcleanup:
    vzeroupper
    add rsp, 160
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
.Ldone:
    ret

.size hexdump_avx, .-hexdump_avx
.att_syntax prefix
