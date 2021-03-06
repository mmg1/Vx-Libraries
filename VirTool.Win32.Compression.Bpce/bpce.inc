NumberOfLeaf equ 512;������������ ���������� ����� � ������
;===============================================================================;
;��������� ����������� ������ ��� ������ ����� ������ ��������
;���� ������ ���������� ��� �������������
;===============================================================================;
Leaf struct
	Left dw ?;���������� ����� ����
	Right dw ?;���������� ������ ����
Leaf ends
;===============================================================================;


;===============================================================================;
;��������� ��������� ������ ������
;===============================================================================;
Header struct
	Signature dw ?;���������. ����� "BP"
	HuffmanCodesSize dw ?;������ ������� ����� ��������
	CompressSize dd ?;������ ������ ������
	DecompressSize dd ?;������������� ������
	CRC32 dd ?;�������� ��� (!) ������ ������
Header ends
;===============================================================================;

;===============================================================================;
;CRC32
;===============================================================================;
CRC32 proc uses esi edi Offset1:DWORD,Size1:DWORD
	mov esi,Offset1
	mov edi,Size1
	cld
	xor     ecx,ecx
	dec     ecx
	mov     edx,ecx
	push    ebx
NextByteCRC:
	xor     eax,eax
	xor     ebx,ebx
	lodsb
	xor     al,cl
	mov     cl,ch
	mov     ch,dl
	mov     dl,dh
	mov     dh,8
NextBitCRC:
	shr     bx,1
	rcr     ax,1
	jnc     NoCRC
	xor     ax,08320h
	xor     bx,0EDB8h
NoCRC:
        dec     dh
	jnz     NextBitCRC
	xor     ecx,eax
	xor     edx,ebx
        dec     edi
	jnz     NextByteCRC
	pop     ebx
	not     edx
	not     ecx
	mov     eax,edx
	rol     eax,16
	mov     ax,cx
	ret
CRC32 endp
;===============================================================================;


;===============================================================================;
;��������� ��� �������������� ������ ������ ����� ������ ��������.
;===============================================================================;
LeafEx struct
	Next dw ?;��������� ����
	Counter dd ?;������� ����� ���������
LeafEx ends
;===============================================================================;

;===============================================================================;
;��������� ��� �������� ���� ��������
;===============================================================================;
HuffmanCode struct
	Length1 db ?;������ ���� � �����
	Code db 32 dup (?);������������ ����� ���� �������� - 32 �����	
HuffmanCode ends
;===============================================================================;

;��� ������ ����� ��������� � ������� ������ , ����� �������� ������� ��� ����. ��� ������ ���� ������� �������� �������� ��� Indexes, Tree, LeafEx
;������ ������ - ��� ������� 14,25 ��
TreeOffset equ 0
TreeExOffset equ 2048
IndexesOffset equ 5120
HuffmanCodesOffset equ 6144

;Tree	Leaf	NumberOfLeaf dup <0>;������ �������� � ������������ ������� ��� �����������
;TreeEx	LeafEx	NumberOfLeaf dup <0>;������ �������� � ��������������� ������� ��� ���������� �����
;Indexes	dw	NumberOfLeaf dup (0);������ �������� ����� ������������� � ������� ����������� ������
;HuffmanCodes 	HuffmanCode 256 dup <0>

;===============================================================================;
;���������� ����� ���������� ����������
;===============================================================================;
TreeInit proc uses esi edi WorkingBuffer:DWORD
	push edi

	mov ecx,NumberOfLeaf;������� ���� ��� ����������
;���������� Tree ���������� ����������. ��������� ��� -1
	mov edi,WorkingBuffer;� edi - ����� Tree
	mov eax,-1; -1 ��������, ��� ������ �� ������ ��� ����� �����������
	rep stosd
;���������� LeafEx.Counter ���������� ����������.
	mov ecx,NumberOfLeaf
	mov edi,WorkingBuffer
	lea edi,[edi+TreeExOffset]
	assume edi:ptr LeafEx
NextCounter:
	mov [edi].Counter,0
	add edi,6
	loop NextCounter
;���������� LeafEx.Next ���������� ����������.
	mov ecx,NumberOfLeaf
	mov edi,WorkingBuffer
	lea edi,[edi+TreeExOffset]
NextNext: 
	mov [edi].Next,-1
	add edi,sizeof LeafEx
	loop NextNext
	pop edi
	ret 
TreeInit endp
;===============================================================================;


;===============================================================================;
;������ �������� ����������� ���������� ���������� - -1 ��� ����
;===============================================================================;
IndexInit proc WorkingBuffer:DWORD
	push edi
	mov ecx,NumberOfLeaf
	mov edi,WorkingBuffer
	lea edi,[edi+IndexesOffset];� EDI - ����� ������� �������� 
	xor eax,eax
	dec eax
	rep stosw;
	pop edi
	ret 
IndexInit endp
;===============================================================================;


;===============================================================================;
;��������� ������� ���������
;===============================================================================;
CountBytes proc uses esi edi WorkingBuffer:DWORD,Buffer:DWORD,size1:DWORD
	push esi
	push edi

	mov esi,WorkingBuffer
	lea esi,[esi+TreeExOffset];� ESI - ����� LeafEx
	xor eax,eax;��������� �������� ����� ��� ��������
	mov ecx,256;���������� �����
NextCode:
	push ecx	
	mov edi,Buffer
	mov ecx,size1
NextByte:
	assume esi:ptr LeafEx
	.if byte ptr [edi]==al
		inc [esi].Counter;����������� ���������� ��������� ��� ������� ���� 
	.endif
	inc edi
	loop NextByte
	pop ecx
	
	add esi,sizeof LeafEx;��������� ��� � ��� �������
	inc ax
	loop NextCode
	
	pop edi
	pop esi
	ret
CountBytes endp
;===============================================================================;

;===============================================================================;
;��������� ������� ���������� ���
;===============================================================================;
BitsSwap proc uses esi edi ebx HCode:DWORD
LOCAL Flag1:DWORD
	mov esi,HCode
	assume esi:ptr HuffmanCode
	xor ecx,ecx
	mov cl,[esi].Length1
	shr ecx,1
	xor edi,edi;������� ����� � ������
	xor ebx,ebx
	mov bl,[esi].Length1;���������� ���
	mov Flag1,0;���� ��� �������� - ��� � ���� ������� ��� ���������
	inc esi
NextBit:
	push ecx

	xor edx,edx
	mov ecx,8
	mov eax,edi
	xor edx,edx
	div ecx
	mov al,[esi][eax];�������� ������ ����
	bt eax,edx;����� ��������� ��� � �����?
	pushf
	mov ecx,edi
	mov eax,ebx
	sub eax,ecx;�������� ��� � ������� ����
	dec eax;��������� � ����
	mov ecx,8
	xor edx,edx
	div ecx
	mov al,[esi][eax]
	bt eax,edx
	jnc IsNull1
	mov Flag1,1
	jmp N1
IsNull1:
	mov Flag1,0
N1:	
	popf
	jnc IsNull;������� �� IsNull ���� ����
	;������ ��� �������
	.if Flag1==1;������ ��� �������, ��������� ��� �������
	;��������� ��� ��
	.endif
	.if Flag1==0;������ ��� �������, ��������� ��� ����
		;�������� �������
		mov eax,edi;����� �������
		mov ecx,8
		xor edx,edx
		div ecx
		push eax
		mov al,[esi][eax]
		btr eax,edx;������������� ������ � ����
		pop ecx
		mov byte ptr [esi][ecx],al

		;����� ����������
		mov ecx,edi
		mov eax,ebx
		sub eax,ecx
		dec eax
		xor edx,edx
		mov ecx,8
		div ecx
		push eax
		mov al,[esi][eax]	
		bts eax,edx
		pop ecx
		mov byte ptr [esi][ecx],al
	.endif
	jmp N2
IsNull:
	;������ ��� �������
	.if Flag1==1;������ ��� �������, ��������� ��� �������
		;�������� �������
		mov eax,edi;����� �������
		mov ecx,8
		xor edx,edx
		div ecx
		push eax
		mov al,[esi][eax]
		bts eax,edx;������������� ������ � ����
		pop ecx
		mov byte ptr [esi][ecx],al

		;����� ����������
		mov ecx,edi
		mov eax,ebx
		sub eax,ecx
		dec eax
		xor edx,edx
		mov ecx,8
		div ecx
		push eax
		mov al,[esi][eax]	
		btr eax,edx
		pop ecx
		mov byte ptr [esi][ecx],al
	.endif
	.if Flag1==0;������ ��� �������, ��������� ��� �������
	;��������� �����
	.endif
N2:
	pop ecx
	inc edi
	dec ecx
	.if ecx>0
		jmp NextBit
	.endif
	ret
BitsSwap endp 
;===============================================================================;

;===============================================================================;
;���������� �� �������� � ���������� ������� �������� ����������� ����������
;===============================================================================;
SetIndexes proc uses edi esi ebx WorkingBuffer:DWORD
	mov edi,WorkingBuffer
	lea esi,[edi+IndexesOffset];� ESI - ����� ������� �������� 
	lea edi,[edi+TreeExOffset];� EDI - ����� TreeEx
;������� ����������.
	assume edi:ptr LeafEx
	mov ecx,256
NextElement:;������� ����
	xor bx,bx;��� �������� ������ �����
	mov eax,[edi].Counter;��� �������� ���������� �� �����
	mov edx,edi;�������� ������ ������ � ������ ���������� ������
	push edi;��� ��������� �� ������� ��������������� ����
	push ecx;�������
	mov ecx,256
	mov edi,WorkingBuffer
	lea edi,[edi+TreeExOffset];� EDI - ����� LeafEx
NextCounter:;���������� ����. ���������� �������� ������ ������� �� ����� ���������.
	.if eax>[edi].Counter
		inc bx
	.endif
	.if (eax==[edi].Counter)&&(edx>edi);���� ������� ���������� � �����������, �� �� ���� ��������� ������ ������.
		inc ebx
	.endif
	add edi,sizeof LeafEx;��������� ������� �������
	loop NextCounter
	pop ecx
	pop edi
	add edi,sizeof LeafEx
	mov word ptr [esi],bx;��������� ������ ��� ����� ��������
	add esi,2;��������� ������� ������� ��������
	loop NextElement
	ret
SetIndexes endp
;===============================================================================;


;===============================================================================;
;�������� ��������� ����� � ������� �������� ��� �������� ����� ����� �������
;���� ��� ������ �������� ���������� 0
;===============================================================================;
NextCount proc uses esi edi WorkingBuffer:DWORD,Offset1:DWORD
	mov esi,Offset1
	mov edi,WorkingBuffer
	lea edi,[edi+IndexesOffset+1024];����� ������� �������� 
	.while esi<edi;���� �� ����� �� ����� �������
		.if word ptr [esi]!=-1
			mov eax,esi
			ret
		.endif
		add esi,2
	.endw
	xor eax,eax
	ret
NextCount endp
;===============================================================================;


;===============================================================================;
;���������� �� �������� ����� �������
;�� �������, ������� �� ������ � ����� ������� - �� ������� ��� ������ ����� -1
;�� �������, ������� ������ � ����� ������� - �� ������� ��� ������ ����� 0
;�.�. ���������� ������ �������� � ���������� ������� ������ ��� ��� �������� ��� ������� �������� ������� ������������� ����� 0
;===============================================================================;
SortIndexes proc uses edi esi ebx WorkingBuffer:DWORD
LOCAL Last:DWORD
LOCAL Count:WORD
	mov esi,WorkingBuffer
	lea edi,[esi+IndexesOffset];��������� �� ������� ��������������� ������� � ������� ��������
	mov esi,edi
;������� ����������. ������� ���������� ����� ��������, �.�. ��� ����� �� ������������ �������������� �������. �� � ������� ��� ������� �� �����.
NextElement:;������� ����
;�������� ����� � ������� �������� ������� �������� ����� ����������� � ����������
	push esi
	push WorkingBuffer
	call NextCount
	.if eax==0
		ret
	.endif
	mov esi,eax
;�������� ������� ��� ������� ������ �� ��������������� �� ����� -1 ������� � ���������
	push eax
	push WorkingBuffer
	call GetCounter
;������� �������� � eax
	mov Count,0;��� �������� ������ �����
	mov edx,esi;�������� ������ ������ � ������ ���������� ������
	mov edi,WorkingBuffer
	lea edi,[edi+IndexesOffset]
	mov ebx,eax;� EBX - ������ �������
NextCounter:;���������� ����. ���������� �������� ������ ������� �� ����� ���������.
	push edi
	push WorkingBuffer
	call NextCount
	.if eax==0
		mov cx,Count
		mov word ptr [esi],cx;��������� ������ ��� ����� ��������
		add esi,2;��������� ������� ������� ��������
		jmp NextElement
	.endif
	mov edi,eax

	push eax
	push WorkingBuffer
	call GetCounter
	.if ebx>eax
		inc Count
	.endif
	.if (ebx==eax)&&(edx>edi);���� ������� ���������� � �����������, �� ������������ � ��� ������, �� �� ���� ��������� ������ ������.
		inc Count
	.endif
	add edi,2;��������� ������� ������� ��������
	jmp NextCounter
SortIndexes endp
;===============================================================================;


;===============================================================================;
;��������� ������� ��� ������� �������� � ������� ��������
;===============================================================================;
GetCounter proc uses esi edi edx WorkingBuffer:DWORD,Offset1:DWORD
	mov edi,WorkingBuffer
	lea esi,[edi+TreeExOffset];� ESI - ����� LeafEx
	lea edi,[edi+IndexesOffset];� EDI - ����� ������� ��������

	mov eax,Offset1
	sub eax,edi;�������� �������� � ������� ��������
	shr eax,1;����� �� 2 � �.�. �������� ����� ��������
	mov ecx,6
	mul ecx;�������� �� 6 � �.�. �������� �������� � ������� TreeEx
	mov eax,dword ptr [esi+2][eax]
	ret
GetCounter endp
;===============================================================================;


;===============================================================================;
;��������� �������� �������� � ������� LeafEx �� ��� �������
;===============================================================================;
GetOffset proc uses esi edi edx ebx WorkingBuffer:DWORD,Index:DWORD
	mov edi,WorkingBuffer
	lea esi,[edi+TreeExOffset];� ESI - ����� LeafEx
	lea edx,[edi+IndexesOffset];� EDX - ����� ������� ��������
;���� ������ - Index � ������� ��������
	mov ecx,NumberOfLeaf
	mov ebx,Index
	lea eax,[edi+IndexesOffset]
NextIndex:
	.if bx==word ptr [edx]
		mov edi,edx
		sub edi,eax;�������� �������� � ������� �������� 
		mov eax,edi

		shr eax,1;�������� ����� �������� � ������� ��������
		mov ebx,6
		mul ebx
		add esi,eax;�������� �������� � TreeEx
		mov eax,esi
		ret
	.endif
	add edx,2
	loop NextIndex
	ret
GetOffset endp
;===============================================================================;


;===============================================================================;
;������������� ���������. ���������� IndexFirst �� �������� �������.
;===============================================================================;
GetIndexFirst proc uses esi edi ebx WorkingBuffer:DWORD
;���� ������ ������
	mov edi,WorkingBuffer
	lea esi,[edi+TreeExOffset];� ESI - ����� LeafEx
	lea edi,[edi+IndexesOffset];� EDI - ����� ������� �������� 
	mov ecx,NumberOfLeaf;������� �����
	xor ebx,ebx;��������� ������
	xor edx,edx;������ ����������� ������ ��������������� ���������� �������� �������
	dec dx;������ ���� ������ ������������, ��� ���� ����� ����� ����-���� ������ ����� �������, �.�. ���� �� �����������.
NextIndex:
	push ecx
	mov bx,word ptr [edi];�������� ������ ���������������� �������
	.if bx<dx
		;��������� �� ������� ��� ������� �������?
		push edi
		push WorkingBuffer
		call GetCounter
		.if eax!=0
			mov dx,bx;��������� ����������� ������
		.endif
	.endif
	add edi,2
	pop ecx
	loop NextIndex
	mov eax,edx	
	ret
GetIndexFirst endp
;===============================================================================;


;===============================================================================;
;�������� ����� �������
;===============================================================================;
CreateNewTable proc uses edi WorkingBuffer:DWORD,IndexFirst:DWORD,IndexLast:DWORD
	mov edi,WorkingBuffer
	lea edi,[edi+IndexesOffset]
	mov ecx,NumberOfLeaf
	mov eax,IndexFirst	
NextIndex:;���� ��� ������� ������, ������� �� ��� ������������ � �� ������������ �� ������ �� ����� 
	.if word ptr [edi]==-1
		jmp x
	.endif 
	.if word ptr [edi]<ax
		mov word ptr [edi],-1;��������� �������� ����������, ��� ��� ������� �� �� ����� � ������, �.�. ��� ��� ���� ������������	
	.else
		mov word ptr [edi],0;��� ��������, �.�. ������� �� �������������� �� ����� ��� ����������
	.endif
x:
	add edi,2
	loop NextIndex
	;�.�. � ������� ������ ���� ��������� ����� �������, �� � ������� �������� ��������� �������������� ��������������� �������, � ������ ������ ������ ��� 0
	mov eax,IndexLast
	lea eax,[eax*2];����� ����� �������� ������ �������� � ������� ��������
	mov edi,WorkingBuffer
	lea edi,[edi+IndexesOffset]
	lea edi,[edi][eax];����� ������ �������� � ������� ��������, ������� �� ����� �������������
	mov word ptr [edi],0
	ret 
CreateNewTable endp
;===============================================================================;

;===============================================================================;
;��������� ������ �������� � TreeEx �� ��������
;===============================================================================;
GetTreeExIndex proc uses edi esi WorkingBuffer:DWORD, Offset1:DWORD
	mov edi,Offset1
	mov esi,WorkingBuffer
	lea esi,[esi+TreeExOffset]
	sub edi,esi;������� 
	xor edx,edx
	mov eax,edi
	mov ecx,6
	div ecx;����� �� 6
	ret
GetTreeExIndex endp
;===============================================================================;


;===============================================================================;
;������ ���������? 1 - ��, 0 - ���
;===============================================================================;
IsRoot proc uses esi edi WorkingBuffer:DWORD
	mov edi,WorkingBuffer
	lea edi,[edi+IndexesOffset]
	mov ecx,NumberOfLeaf
	xor eax,eax
NextLeaf: 
	.if word ptr [edi]!=-1
		inc eax
	.endif 
	add edi,2
	loop NextLeaf
	.if eax==1
		ret		
	.else
		xor eax,eax	
		ret
	.endif
IsRoot endp
;===============================================================================;


;===============================================================================;
;����� � ������� Tree �� ������ ��������
;===============================================================================;
GetAddress proc uses esi edi WorkingBuffer:DWORD,Number:DWORD
	mov esi,Number
	shl esi,2;�������� �� 4
	mov edi,WorkingBuffer 
	lea edi,[edi+TreeOffset]
	add edi,esi
	mov eax,edi
	ret
GetAddress endp
;===============================================================================;


;===============================================================================;
;����� ������� ��� �������������
;===============================================================================;
AddNullBit proc uses esi edi HCode:DWORD
	mov esi,HCode
	assume esi:ptr HuffmanCode
;��������� � ����� ����� ��� ���
	xor eax,eax
	mov al,[esi].Length1
	dec eax
	mov ecx,8
	xor edx,edx
	div ecx;����� �� 8, ����� �������� ����� �����(� ��� �� 0-3) � ����� ���� � edx
	push eax
	mov al,[esi+1][eax];�������� ������ ����
	mov ecx,1
	xchg ecx,edx
	shl dl,cl;1 shl (����� ���� � �����)
	not dl
	and al,dl
	pop edi
	mov byte ptr [esi+1][edi],al;���������� � 0 [esi].Length ���	
	ret
AddNullBit endp
;===============================================================================;


;===============================================================================;
;����� ��������� ��� �������������
;===============================================================================;
AddOneBit proc uses esi edi HCode:DWORD
	mov esi,HCode
	assume esi:ptr HuffmanCode
;��������� � ����� ����� ��� ���
	xor eax,eax
	mov al,[esi].Length1
	dec eax
	mov ecx,8
	xor edx,edx
	div ecx;����� �� 8, ����� �������� ����� �����(� ��� �� 0-3) � ����� ����
	push eax
	mov al,[esi+1][eax];�������� ������ ����
	mov ecx,1
	xchg ecx,edx
	shl dl,cl;1 shl (����� ���� � �����)
	or al,dl
	pop edi
	mov byte ptr [esi+1][edi],al;���������� � 0 [esi].Length-1 ���	
	ret
AddOneBit endp
;===============================================================================;


;===============================================================================;
;���������� ������� ������������ ������
;===============================================================================;
GetTreeSize proc WorkingBuffer:DWORD
	;������� ���������� ����� � ������
	;��� ����� ����� ������� TreeEx � ���������. � ��� ��� ������� ��������� �� � �����. ������ 256 �� �����.
	mov esi,WorkingBuffer
	mov eax,256
	mov ecx,6
	mul ecx;�������� � TreeEx
	lea esi,[esi][eax];����� � TreeEx
	mov ecx,256
	xor eax,eax
	assume esi:ptr LeafEx
NextCount1:
	.if [esi].Counter!=0
		inc eax
	.endif
	add esi,sizeof LeafEx
	loop NextCount1   
	
	ret
GetTreeSize endp
;===============================================================================;


;===============================================================================;
;�������� ������� ����� �� ������ String1 � ������� ��������� Offset1
;� �������� ����� �� ������ String2 � � ������� ��������� Offset2.
;���������� ����� ������� ���� ���������� ����� Number. ��� ����� ����� � String2
;===============================================================================;
CmpBits proc uses esi edi ebx String1:DWORD,Offset1:DWORD,String2:DWORD,Offset2:DWORD,Number:DWORD
LOCAL Flag1:BYTE
LOCAL Flag2:BYTE
	mov esi,String1;� ��� ����������
	mov edi,String2;��� ����������
	mov ecx,Number;������� ����� ����������
NextBit:
	push ecx
;�������� ���� String1 �� �������� �������� Offset1
	mov eax,Offset1
	mov ecx,8
	xor edx,edx
	div ecx;� EAX - ����� �����, � EDX - ����� ���� � �����
	mov al,byte ptr [esi][eax];��� � ������ ����
	and eax,0FFh;�������� ��� ����� al � eax
	bt eax,edx;������ ����� � ������ ���� ��� �������
	jnc IsNull;������� ���� ����
	mov Flag1,1;� ������ ������� ������ ��������� ��� ����� �������
	jmp NextByte
IsNull:
	mov Flag1,0;� ������ ������� ������ ��������� ��� ����� ����
;�������� ���� String2 �� �������� �������� Offset2
NextByte:
	mov eax,Offset2
	mov ecx,8
	xor edx,edx
	div ecx;� EAX - ����� �����, � EDX - ����� ���� � �����
	mov al,byte ptr [edi][eax];��� � ������ ����
	and eax,0FFh;�������� ��� ����� al � eax
	bt eax,edx;������ ����� � ������ ���� ��� �������
	jnc IsNull2;������� ���� ����
	mov Flag2,1;�� ������ ������� ������ ��������� ��� ����� �������
	jmp NextNext
IsNull2:
	mov Flag2,0;�� ������ ������� ������ ��������� ��� ����� ����
NextNext:
	mov al,Flag2
	.if Flag1==al;���� �����
		pop ecx
		inc Offset1
		inc Offset2
		loop NextBit
		xor eax,eax
		inc eax
		ret
	.else
		pop ecx
		xor eax,eax;������� ������ �� �����
		ret
	.endif
CmpBits endp
;===============================================================================;


;===============================================================================;
;���������� ������� ������������ ������
;===============================================================================;
GetCodeSize proc uses esi edi WorkingBuffer:DWORD
LOCAL Summa:DWORD
	mov ecx,256
	xor eax,eax
	mov edi,WorkingBuffer
	lea esi,[edi+TreeExOffset];����� ��� ��������� ������
	lea edi,[edi+HuffmanCodesOffset];����� �������������
	assume esi:ptr LeafEx
	assume edi:ptr HuffmanCode
	mov Summa,0
NextByte: 
	mov eax,[esi].Counter
	xor edx,edx
	mov dl,[edi].Length1	
	mul edx;�������*(�����_����_�_�����)
	add Summa,eax
	add esi,6
	add edi,33
	loop NextByte
	add Summa,7
	mov eax,Summa
	xor edx,edx
	mov ecx,8
	div ecx;��������� �� 8
	ret
GetCodeSize endp
;===============================================================================;

;===============================================================================;
;���������� ����� ��������
;Root - ����� ��������, ��� ������
;CodeOffset - ����, ������������� �������� �������� 
;pHCode - ��������� �� ��� ����������� ������ �����
;WorkingBuffer - ������� �����
;===============================================================================;
GetHuffmanCodes proc uses esi edi eax ecx ebx edx WorkingBuffer:DWORD,CodeOffset:DWORD,pHCode:DWORD,Root:DWORD
LOCAL HCode:HuffmanCode
	mov ecx,sizeof HCode
	mov esi,pHCode
	lea edi,HCode
	rep movsb;� ���� ��������� ���������� HCode �������� 33 ����� � ������ pHCode. 
	mov esi,Root
	assume esi:ptr Leaf
	.if ([esi].Left==-1)&&([esi].Right==-1)
		;��������� ������ esi
		mov edi,WorkingBuffer
		lea edi,[edi+TreeOffset]
		sub esi,edi;������� 
		shr esi,2;����� �� 4 �������� ������
		.if esi==CodeOffset;��� ��������
			;�������� �� HCode � ������ HuffmanCodes[CodeOffset]
			mov eax,CodeOffset
			mov ecx,33
			mul ecx;�������� �� 33
			mov edi,WorkingBuffer
			lea edi,[edi+HuffmanCodesOffset][eax];� ESI ����� ������� ��������
			mov ecx,sizeof HCode
			lea esi,HCode
			rep movsb;�������� ����� ������������� � ������
			ret
		.endif
		ret
	.endif
	lea edi,HCode
	assume edi:ptr HuffmanCode
	inc [edi].Length1;����������� ����� �������������
	.if [esi].Left!=-1
		push edi;����� HuffmanCode
		call AddNullBit;����� ������� ��� �������������
		
		xor eax,eax
		mov ax,[esi].Left
		push eax
		push WorkingBuffer
		call GetAddress;��������� �� [esi].Left ��� �����

		push eax
		lea ecx,HCode
		push ecx
		push CodeOffset
		push WorkingBuffer
		call GetHuffmanCodes
	.endif
	.if [esi].Right!=-1
		push edi;����� HuffmanCode
		call AddOneBit;����� ��������� ��� �������������

		xor eax,eax
		mov ax,[esi].Right
		push eax
		push WorkingBuffer
		call GetAddress;

		push eax
		lea ecx,HCode
		push ecx
		push CodeOffset
		push WorkingBuffer
		call GetHuffmanCodes
		ret
	.endif
	ret
GetHuffmanCodes endp
;===============================================================================;


;===============================================================================;
;����������� �����
;CompressData - ���� ��������
;BitsString - ��� ��������
;Number - ������� ��� ��������
;BitsOffset - �� ������ �������� �������� ��������
;===============================================================================;
CopyBits proc uses esi edi ebx CompressData:DWORD,BitsOffset:DWORD,BitsString:DWORD,Number:DWORD
LOCAL ByteOffset:DWORD
	mov esi,CompressData
	mov ecx,Number
	mov edi,BitsString
	xor ebx,ebx;������� �������� � �������� ������ �����
NextBit:
	push ebx
	push ecx

	mov eax,ebx
	mov ecx,8
	xor edx,edx;������� ����� ��������
	div ecx;�������� ����� ����� � ������� ���������� ��� ������� ���� ���������
	mov al,byte ptr [edi][eax];�������� ���� � ������� ���������� ������ ���
	bt eax,edx;��������� ���� ��� ��� �������
	pushf;��������� ������� ������
	mov eax,BitsOffset
	mov ecx,8
	xor edx,edx;������� ����� ��������
	div ecx;�������� ���� � ������� ���� ���������� ����
	mov ByteOffset,eax;��������� �������� ��������
	mov al,byte ptr [esi][eax]
	popf
	jnc IsNull;���� ��� ����, �� ������� �� IsNull
	bts eax,edx;��������� ������ ��� � 1
	jmp NextNext
IsNull:
	btr eax,edx;��������� ������ ��� � 0
NextNext:
	mov ebx,ByteOffset
	mov byte ptr [esi][ebx],al
	inc BitsOffset
	pop ecx
	pop ebx
	inc ebx;��������� ���
	loop NextBit
	ret
CopyBits endp
;===============================================================================;


;===============================================================================;
;����������� ������
;===============================================================================;
Compressing proc WorkingBuffer:DWORD,Buffer:DWORD,Size1:DWORD,CompressData:DWORD
	mov esi,WorkingBuffer
	mov edi,Buffer
	lea esi,[esi+HuffmanCodesOffset]
	mov ecx,Size1
	xor ebx,ebx;������� �������� ���������
NextByte:
	push ecx
	xor eax,eax
	mov al,byte ptr [edi];�������� ���� ������� ���� ������������
	;���� ������������ � ������� ����� ��������
	mov ecx,33
	mul ecx;�������� �������� � ������� ����� ��������
	lea edx,[esi][eax];����� �������� � ������� ����� ��������
	assume edx:ptr HuffmanCode

	xor eax,eax
	mov al,[edx].Length1
	push edx;��������� EDX

	push eax;������� ����� ���� ���������
	inc edx
	push edx;��� ���� ���������
	push ebx;������� �������� ���������
	push CompressData;���� ����������
	call CopyBits

	pop edx;��������������� EDX
	xor eax,eax
	mov al,byte ptr [edx]
	add ebx,eax

	inc edi;�������� ��������� ����
	pop ecx
	loop NextByte
	ret
Compressing endp
;===============================================================================;


;===============================================================================;
;������� ����������� ������� ����� ��������
;���������� ������ ����������� �������
;===============================================================================;
CreatePackHuffmanCode proc uses edi esi WorkingBuffer:DWORD,Buffer:DWORD
	mov edi,WorkingBuffer
	lea edi,[edi+HuffmanCodesOffset];� EDI - ������� ����� ��������
	assume edi:ptr HuffmanCode
	mov esi,Buffer;� ESI - ���� ����� ������ ����������� ������� ����� ��������
	mov ecx,256
	xor eax,eax;����� �������� ������������ ����
;������� ������� �����������
NextByte:
	.if [edi].Length1!=0;���� ��� ����� ����� ��� ����, �� ��������� ��� � ������� �����������
		push ecx
		push eax;��������� ����� ���� 
		mov ecx,8
		xor edx,edx
		div ecx;� EAX - ����� �����, � EDX - ����� ����
		push eax
		mov al,byte ptr [esi][eax];�������� ������ ���� � ������� �����������
		and eax,0FFh
		bts eax,edx;������������� ����� ���� � ������� �����������
		mov edx,eax
		pop ebx
		mov byte ptr [esi][ebx],dl;��������� ���� � ������� �����������
		pop eax;��������������� ������� �����
		pop ecx
	.endif
	add edi,sizeof HuffmanCode
	inc eax
	loop NextByte
;������� ����������� ������� ����� ��������	
;������� ���� ����� ���� ��������. ��� �������� 1 ����.
;����� ���� ��� ��� � ��� ���� ����������� �� �����.
	mov ecx,256
	mov edi,WorkingBuffer
	lea edi,[edi+HuffmanCodesOffset]
	xor edx,edx;��������� ������� ��� ����������� ������� ����� ��������
	mov edx,32;����� ����� � ����� ����� ������ ����������� �������
NextCode:
	push ecx
	.if [edi].Length1!=0
		mov al,[edi].Length1
		mov byte ptr [esi][edx],al;���������� ����� ���� � ������� ����������� �����
		inc edx;��������� ����
		;�������� al ��� �� ������ [esi][edx] � ������� ��������� 0 �� [edi+1]
		push edx;��������� ������� ������

		push eax;������� ���
		lea ebx,[edi+1]
		push ebx;������ ����������
		push 0;�������� 0
		lea ebx,[esi][edx]
		push ebx;���� ����������
		call CopyBits

		
		;������� ���� �������� ��� ��� ������� �����
		mov al,[edi].Length1;����� ���� � �����
		mov ecx,8

		xor edx,edx
		div ecx;�������� ���������� ����. ����������� ���������� = EAX + 1
		inc eax;����������� ����������
		pop edx;��������������� ������� ������
		
		
		add edx,eax;��������� ����� � ����������� �������
	.endif 
	add edi,sizeof HuffmanCode
	pop ecx
	loop NextCode
	mov eax,edx
	ret
CreatePackHuffmanCode endp
;===============================================================================;


;===============================================================================;
;���������� ������ �� ������ ��������
;===============================================================================;
bp_decompress proc Buffer:DWORD,CompressData:DWORD,WorkingBuffer:DWORD
LOCAL Offset1:DWORD;�������� � ������������� ������
	mov Offset1,0
;�������� ������ 256*33 ����, �.�. ������� �����
	mov ecx,256*33
	mov edi,WorkingBuffer
	mov eax,0
	rep stosb

	mov edi,CompressData
	assume edi:ptr Header
	.if [edi].Signature!="PB"
		xor eax,eax
		ret;����������� ���������� ���������
	.endif
	mov ebx,[edi].CRC32

	mov [edi].CRC32,0

	push [edi].CompressSize
	push edi
	call CRC32
	.if eax!=ebx
		xor eax,eax
		ret;������������ CRC32
	.endif
	mov [edi].CRC32,eax
	add edi,sizeof Header;������ � EDI - ����������� ������� ����� ��������, ������ 32 ���� ������� - ������� �����������
	mov ebx,edi;
	add ebx,32;����������� ������� ����� ��������
;1)������������� ����������� ������� ��������
	mov esi,WorkingBuffer;������������� �������
	mov ecx,256
	xor eax,eax;������� ��� ����� ������� �������������
NextCode:
	push ecx
	push eax 
	
	mov ecx,8
	xor edx,edx
	div ecx

	mov al,byte ptr [edi][eax];�������� ���� ������� �����������
	bt eax,edx
	jnc IsNull
	;���� �� ����, ��...
	pop eax
	push eax
	
	mov al,byte ptr [ebx];����� ����
	mov byte ptr [esi],al;���������� ����� ����
	inc esi
	inc ebx

	push eax

	push eax;������� ���
	push ebx;������ ����������
	push 0;�������� 0
	push esi;���� ����������
	call CopyBits;������ ���������� ��� ���

	pop eax;���������� ���
	mov ecx,8
	xor edx,edx
	div ecx;���������� ����
	inc eax;���������� ���� + 1
	add ebx,eax;��������� ����� � ������� �����
	dec esi
	add esi,sizeof HuffmanCode
	pop eax
	pop ecx
	jmp NextNext
IsNull:
	pop eax
	pop ecx
	add esi,sizeof HuffmanCode
NextNext:
	inc eax
	loop NextCode
;�������������
	mov edi,CompressData
	mov ecx,[edi].DecompressSize
	xor ebx,ebx
	mov bx,[edi].HuffmanCodesSize
	lea edi,[edi+sizeof Header][ebx];��������� �� ������ ������
	xor ebx,ebx;������� ��������� �� ������ ������

NextByte: 
	push ecx
	;������ ��� ���������� � ���������� �� �������������� ������
	mov ecx,256
	assume esi:ptr HuffmanCode
	mov esi,WorkingBuffer;������� ����� ��������
NextCode1:
	push ecx
	.if [esi].Length1!=0;��������� ��� ������
		;���������� ���� ��� � ����� ������� �������
		xor eax,eax
		mov al,[esi].Length1
		push eax;����� ������������ ������
		push 0;�������� � ���� �������, �.�. �� ���������� � �����
		inc esi
		push esi;� ��� ����������
		dec esi
		push ebx;�� �������� ebx
		push edi;��� �����������
		call CmpBits
		.if eax==1;����� �������������
			xor eax,eax
			
			mov al,byte ptr [esi]
			add ebx,eax;����� ������� ��������

			push edi
			push esi

			;����� �������� � ������� ����� ��������
			mov edi,WorkingBuffer
			sub esi,edi;�������
			mov ecx,33
			xor edx,edx
			mov eax,esi
			div ecx;� eax - ����� ����� � => ����� ��������

			mov edi,Buffer
			mov edx,Offset1
			mov byte ptr [edi][edx],al
			inc Offset1

			pop esi
			pop edi
			pop ecx
			jmp GoodByte
		.endif
	.endif
	pop ecx
	add esi,33;��������� ���
	loop NextCode1
GoodByte:
	pop ecx
	loop NextByte
	xor eax,eax
	inc eax
	ret
bp_decompress endp
;===============================================================================;


;===============================================================================;
;������ ������ �� ������ ��������
;===============================================================================;
bp_compress proc uses edi esi WorkingBuffer:DWORD,Buffer:DWORD,size1:DWORD,CompressData:DWORD
LOCAL IndexLast:DWORD
LOCAL IndexFirst:DWORD
LOCAL IndexNewLeaf:DWORD
LOCAL HCode:HuffmanCode
LOCAL Flag:DWORD
LOCAL Flag2:DWORD
;1) ��������� ������ ����� ���������� ����������
	push WorkingBuffer
	call TreeInit
;2) ��������� ������ ��������
	push WorkingBuffer
	call IndexInit
;3) ������������ ����� ��������� ������
	push size1
	push Buffer
	push WorkingBuffer
	call CountBytes
;4) ���������� �� ����������� ������
	push WorkingBuffer
	call SetIndexes
;5�) ������������� IndexFirst
	push WorkingBuffer
	call GetIndexFirst
	mov IndexFirst,eax
;5�) ������������� IndexLast
	mov IndexLast,0FFh
;6) �������� �� �������� ������� ��� ����� ���� � ���������� � �����
NextLeaf:;��������� ����
;��������� ����� 1-�� �������� � ����������� ��������
	push IndexFirst
	push WorkingBuffer
	call GetOffset
	push eax;��������� ����� 1-�� �������� � ����������� ��������
;��������� ����� 2-�� �������� � ����������� �������� IndexFirst + 1
	mov eax,IndexFirst
	inc eax
	push eax
	push WorkingBuffer
	call GetOffset
;�������� ������ ������ ��� ���������
	pop edi;��������������� ����� 1-�� �������� � ����������� ��������
	mov esi,eax;����� 2-�� �������� � ����������� ��������
	xchg esi,edi
	;ESI - ����� � ����������� ��������
	;EDI - ����� � ����������� �������� ����� EDI

	assume esi:ptr LeafEx
	assume edi:ptr LeafEx
;��������� ����� ������� ���������� �������� ��� ���������� ������ ���� � ������. ������ ��������� ������� ����� ����� ������ IndexLast + 1
	push edi
	inc IndexLast
	mov eax,IndexLast;�������� �������� IndexLast + 1
	mov ecx,6
	xor edx,edx
	mul ecx;�������� �� 6

	mov edi,WorkingBuffer
	lea edi,[edi+TreeExOffset][eax]
	mov eax,edi
	assume eax:ptr LeafEx
	pop edi
;��������� ���� ��� ������ ����
	xor edx,edx
	mov edx,[esi].Counter
	add edx,[edi].Counter;����� ������ ��� ������ ����
	mov ecx,[esi].Counter
;������� ��� ������ ����
	mov [eax].Counter,edx
;��������� ������ �� EDI
	push edi
	push WorkingBuffer
	call GetTreeExIndex
	mov ecx,eax
	
	mov eax,IndexLast
	shl eax,2;�������� �� 4
	mov edi,WorkingBuffer
	lea edi,[edi][eax]
	assume edi:ptr Leaf
;��������� ����� ������� � ������
	mov [edi].Left,cx
;��������� ������ �� ESI
	push esi
	push WorkingBuffer
	call GetTreeExIndex
	mov ecx,eax
;��������� ������ ������� � ������
	mov [edi].Right,cx
;��������� ����� ������� ������
	add IndexFirst,2
;��� �������� ����� ������� ������ ������� �������� ����� �����, �.�. ��� ����������� �������� ������������ ������� ����������
;� ��������� CreateNewTable �������� IndexFirst. � ����� ������� �� ��������� �������� � ��������� ������ IndexFirst + 1
;���������� ����� ���� ��� - ���� ������ ������ -1, �� ������� ������� ��� ������������� ��� �������������� ��� �������� ������ ����. 
;������ ��� ������� �� �� �����. ����� ���� ������ ����� ����, �� �������� �������������� ���� ����� ����������� �� ��������. 
	push IndexLast
	push IndexFirst
	push WorkingBuffer
	call CreateNewTable
;��������� ����� ������� �� ����������� ������ ���������
	push WorkingBuffer
	call SortIndexes
;��������� ������� ����� � �������
	push WorkingBuffer
	call IsRoot
	.if eax==1
		mov esi,WorkingBuffer
		lea esi,[esi+IndexesOffset]
		mov ecx,512
NextByte: 
		.if word ptr [esi]==0
			jmp Ok;������ ����� � ESI
		.endif
		add esi,2
		loop NextByte
Ok:
		;�������� ����� � Tree ��������� ��������
		;��������� ���� �������� � ���������� �� � �����
		mov eax,512
		sub eax,ecx;� EAX - ������ ��������� ��������
		mov edi,WorkingBuffer;�������� ����� �����
		lea edi,[edi+TreeOffset]
		shl eax,2;�������� eax �� 4
		lea edi,[edi][eax]

		mov ecx,256
		xor esi,esi
		mov ebx,edi;����� �����
NextPred:;��������� �������������
		push ebx

		push ecx
		;�������� HCode
		lea edi,HCode
		mov eax,0
		mov ecx,33
		rep stosb
		pop ecx


		lea eax,HCode
		push eax
		push esi;����� ���� ����, ���� ������� ���� �� ���� ��������� ������
		push WorkingBuffer
		call GetHuffmanCodes
		push ecx
		;������ ������� ��� � �������������
		mov edi,WorkingBuffer
		lea edi,[edi+HuffmanCodesOffset]
		mov ecx,33
		mov eax,esi
		mul ecx
		lea edi,[edi][eax]
		assume edi:ptr HuffmanCode
		;.if [edi].Length1!=0
		;	push edi
		;	call BitsSwap
		;.endif
		inc esi
		pop ecx
		loop NextPred
		;����� ������� ��������� |���������|�������_�����_��������|������|. ��� ��������� � ����� ������� �������.
		push WorkingBuffer
		call GetCodeSize;���������� ������� ������������ ������
		mov ecx,eax
		add ecx,sizeof Header
		add ecx,32
		.if ecx>size1;���� ������ ������ ������ ��������� ������ �������� ������, �� ������
			xor eax,eax
			ret
		.endif
		;������� ���������
		mov edi,CompressData
		assume edi:ptr Header
		push edi
		mov [edi].Signature,"PB"
		mov [edi].CompressSize,eax
		add [edi].CompressSize,sizeof Header;��������� � ������� ������ ������ ���������



		mov [edi].CRC32,eax
		mov eax,size1
		mov [edi].DecompressSize,eax


		lea esi,[edi+sizeof Header];����� ������ ���� ����������� ������� ����� ��������
		
		push esi
		push WorkingBuffer
		call CreatePackHuffmanCode;������� ������ ����������� ����� ��������
		mov [edi].HuffmanCodesSize,ax
		add [edi].CompressSize,eax
		add esi,eax;�������� ��� ������ ������

		push esi;����� ��� ������ ������
		push size1;������ �������� ������
		push Buffer;�������� ������
		push WorkingBuffer;������� �����
		call Compressing;��������� �����������
		;���������� CRC32
		pop edi
		mov ecx,[edi].CompressSize
		mov [edi].CRC32,0
		push ecx
		push edi
		call CRC32
		mov [edi].CRC32,eax
		ret;����� �������� ������
	.else
		mov IndexFirst,0;IndexFirst ����� 0, IndexLast ����� ����� �� 
		jmp NextLeaf;��������� ����
	.endif
bp_compress endp
;===============================================================================;