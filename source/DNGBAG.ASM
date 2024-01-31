; DN GASBAG
;
; (c) 2024 Branislav Juhas
;
; Version 0.1 alpha
;
; The earliest version of the game, the code is very messy and unoptimized,
; barely working. The game experiences a lag every few frames and the keyboard
; input sometimes doesn't work. The game is not playable at this stage, the code
; is only for experimentation with different code design approaches.

.model small            ; small memory model
.stack 100h             ; 256 bytes stack

.data

    balloon STRUC      ; structure for balloon
        x dw ?          ; x coordinate
        y dw ?          ; y coordinate
        collision dw ?  ; y coordinate of collision
    ENDS                ; end of structure

    bullet STRUC       ; structure for bullet
        x dw ?          ; x coordinate
        y dw ?          ; y coordinate
        collision dw ?  ; y coordinate of collision
    ENDS

    ; create 10 balloons as an array
    balloons balloon 10 dup (<>)

    ; create 5 bullets as an array
    bullets bullet 5 dup (<>)

    shooter dw 0        ; x coordinate of pad
    score dw 0          ; score

    last dw 0           ; last time
    delta dw 0          ; delta time

    copyright db "(C) Branislav Juhas 2024"
    version db "DN GASBAG v0.1 (Alpha)"

.code
start:
    mov ax, @data       ; initialize the data segment
    mov ds, ax          ; initialize the data segment
print:  
    mov cx, 0           ; clear cx register for counting

    mov ah, 0           ; set video mode
    mov al, 3           ; 80x25 color text mode
    int 10h             ; call BIOS video interrupt

    mov ah, 2           ; set cursor position
    mov bh, 0           ; page number
    mov dh, 0           ; row
    mov dl, 0           ; column
    int 10h             ; call BIOS video interrupt

    ; set color
    mov ah, 0bh         ; set background and foreground color
    mov bh, 0           ; page number
    mov bl, 11101111b   ; background and foreground color
    int 10h             ; call BIOS video interrupt

    mov si, offset copyright
    mov ax, 0b800h      ; video memory segment
    mov es, ax          ; set es to video memory segment
    mov al, 0           ; clear al register
    mov ah, 15          ; set ah register to 15 for white color

    ; set cx to the length of the string
    mov cx, 24          ; number of characters to print
    mov di, 0f70h       ; set di to the end of the screen
    copyright_loop:
        lodsb           ; load character from si to al
        stosw           ; store data from ax to es:di
        loop copyright_loop ; loop until cx is 0

    ; print version using video memory segment
    mov si, offset version
    mov ax, 0b800h      ; video memory segment
    mov es, ax          ; set es to video memory segment
    mov al, 0           ; clear al register
    mov ah, 15          ; set ah register to 15 for white color

    ; set cx to the length of the string
    mov cx, 22          ; number of characters to print
    mov di, 0f00h       ; set di to the end of the screen
    version_loop:
        lodsb           ; load character from si to al
        stosw           ; store data from ax to es:di
        loop version_loop ; loop until cx is 0

    mov ax, 1
    push ax             ; push 1 to stack for PAD proc
    call PAD

    ; Get timer tick count
        mov ah, 2ch         ; set ah to 2ch for timer tick count
        int 21h             ; call DOS interrupt
        mov last, dx        ; save timer tick count to last

    main_loop:
        ; Get timer tick count
        mov ah, 2ch         ; set ah to 2ch for timer tick count
        int 21h             ; call DOS interrupt
        mov cx, dx          ; save timer tick count to cx
        sub dx, last        ; subtract last from dx
        mov delta, dx       ; save delta time to delta
        mov last, cx        ; save timer tick count to last

        cmp dx, 0           ; if delta time is 0
        ;je main_loop        ; jump to main loop

        ; output delta time to screen with 4 digits
        mov ax, 0b800h      ; video memory segment
        mov es, ax          ; set es to video memory segment
        mov di, 0           ; set di to the beginning of the screen
        mov ah, 15          ; set ah register to 15 for white color
        mov al, 0           ; clear al register

        ; devide dx by 1000
        mov ax, delta       ; load delta time to ax
        mov bx, 1000        ; set bx to 1000
        xor dx, dx          ; clear dx register
        div bx              ; divide ax by bx
        mov ah, al          ; move ah to al
        mov ah, 15          ; set ah register to 15 for white color
        add al, 30h         ; add 30h to al
        stosw               ; store data from ax to es:di
        mov ax, dx          ; move dx to ax   
        mov bx, 100         ; set bx to 100
        xor dx, dx          ; clear dx register
        div bx              ; divide ax by bx
        mov ah, al          ; move ah to al
        mov ah, 15          ; set ah register to 15 for white color
        add al, 30h         ; add 30h to al
        stosw               ; store data from ax to es:di
        mov ax, dx          ; move dx to ax
        mov bx, 10          ; set bx to 10
        xor dx, dx          ; clear dx register
        div bx              ; divide ax by bx
        mov ah, al          ; move ah to al
        mov ah, 15          ; set ah register to 15 for white color
        add al, 30h         ; add 30h to al
        stosw               ; store data from ax to es:di
        mov ax, dx          ; move dx to ax
        mov ah, al          ; move ah to al
        mov ah, 15          ; set ah register to 15 for white color
        add al, 30h         ; add 30h to al
        stosw               ; store data from ax to es:di

        in al, 60h      ; read keyboard input
        cmp al, 1       ; if key 1 is pressed
        je exit         ; exit program
    
        cmp al, 75      ; if left arrow key is pressed
        je left         ; jump to left

        cmp al, 77      ; if right arrow key is pressed
        je right        ; jump to right

        jmp always      ; jump to always

        left:
            mov ax, 0   ; set al to 1 for left
            push ax     ; push 1 to stack for PAD proc
            call PAD    ; call PAD proc
            jmp always  ; jump to always

        right:
            mov ax, 2   ; set al to 2 for right
            push ax     ; push 2 to stack for PAD proc
            call PAD    ; call PAD proc
            jmp always  ; jump to always

        always:

        jmp main_loop   ; jump to main loop

exit:
    mov ah, 4ch         ; exit program
    int 21h             ; call DOS interrupt

PAD proc
    ARG shift:byte    

    mov ax, shooter ; load x coordinate of pad to ax
    ; divide ax by 0333h
    mov bx, 0333h   ; set bx to 0333h
    xor dx, dx      ; clear dx register
    div bx          ; divide ax by bx
    mov bx, 2       ; set bx to 2
    mul bx          ; multiply ax by bx
    cmp ax, 2       ; if x coordinate is less than 2
    jle short_jump_int  ; jump to short label
    sub ax, 2       ; subtract 2 from x coordinate

    short_jump_int:     ; short label for short jump

    add ax, 0e60h   ; add 0e60h to ax
    mov di, ax      ; set di to ax
    mov ax, 0b800h  ; video memory segment

    mov es, ax      ; set es to video memory segment
    mov ah, 15      ; clear ah register
    mov al, 0       ; set al to character c4h
    stosw           ; store data from ax to es:di
    stosw           ; store data from ax to es:di

    mov ah, shift   ; load shift to ax
    cmp ah, 1       ; if shift is 1
    je render       ; jump to render if shift is 1

    mov ax, delta   ; load x coordinate of pad to ax
    mov bx, shooter ; load delta time to bx
    mov cx, 50      ; set cx to 10
    mul cx          ; multiply ax by cx
    
    jg move_right            ; jump to right
    jl move_left             ; jump to left

    render:
        mov ax, shooter ; load x coordinate of pad to ax
        ; divide ax by 0333h
        mov bx, 0333h   ; set bx to 0333h
        xor dx, dx      ; clear dx register
        div bx          ; divide ax by bx
        mov bx, 2       ; set bx to 2
        mul bx          ; multiply ax by bx
        cmp ax, 2       ; if x coordinate is less than 2
        jle short_jump  ; jump to short label
        sub ax, 2       ; subtract 2 from x coordinate
    
        short_jump:     ; short label for short jump

        add ax, 0e60h   ; add 0e60h to ax
        mov di, ax      ; set di to ax
        mov ax, 0b800h  ; video memory segment

        mov es, ax      ; set es to video memory segment
        mov ah, 15      ; clear ah register
        mov al, 0c4h    ; set al to character c4h
        stosw           ; store data from ax to es:di
        stosw           ; store data from ax to es:di
        mov al, 0d0h    ; set al to character c5h
        stosw           ; store data from ax to es:di
        mov al, 0c4h    ; set al to character c6h
        stosw           ; store data from ax to es:di
        stosw           ; store data from ax to es:di
        stosw           ; store data from ax to es:di
        stosw           ; store data from ax to es:di
        stosw           ; store data from ax to es:di

    jmp end_pad

    move_left:
        sub bx, ax      ; subtract ax from bx
        mov shooter, bx ; save x coordinate of pad to shooter
        jmp render      ; jump to render

    move_right:
        add bx, ax      ; add ax to bx
        mov shooter, bx ; save x coordinate of pad to shooter
        jmp render      ; jump to render

    end_pad:
    ret
PAD endp

end start