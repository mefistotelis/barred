program barred;
USES CRT,DOS;

VAR F1:FILE OF CHAR;
    X,Y,Z:CHAR;
    a:LongInt;	{Pozycja w linii}
    b:LongInt;	{Linia(na ekranie) w ktorej stoi kursor}
    N:LongInt;	{Ilosc znakow w linii}
    O:LongInt;	{Ilosc linii powyzej ekranu}
    P:LongInt;	{Liczba odjetych bajtow}
    C,J,K:LONGINT;
    M:Byte;     {Jak bardzo trzeba zmieni† zawarto˜† ekranu(od˜wierzy†)
                0-  nic nie robi†
                1-  od˜wierzy† pasek stanu(g¢rny)
                2-  od˜wierzy† caˆy wy˜wietlony plik
                4-  nieu¾yte
                8-  aktualn¥ lini©
                16- aktualn¥ lini© i poni¾ej
                32- lini© pierwsz¥, reszt© przesuä w d¢ˆ
                64- lini© ostatni¥, reszt© przesuä w g¢r©
                128-od˜wierzy† pasek menu(dolny)}
    Opcje:Byte;{1- Zamieniaj 0 na spacj©
    }
    I:INTEGER;
    name,Nam1:string;


PROCEDURE HELPME;
BEGIN
 WRITELN;
 WRITELN('     Games Editor                     (c) by Tomasz Lis');WRITELN;
 WRITELN('Program umo¾liwia edycje plik¢w gier,');WRITELN('kt¢re zawieraj¥ informacje o etapach lub podobne.');
 WRITELN;WRITELN('  Wywoˆanie:');WRITELN('     HEDIT [file] [number]');WRITELN('  Gdzie:');
 WRITELN(' [file]   - Nazwa pliku do edycji,');WRITELN(' [number] - Numer bajtu pliku, kt¢ry b©dzie oznaczony jako pierwszy.');
 WRITELN('               Opcje edycji:');
 WRITELN(' <up arrow>/<down arrow>    -Linia w g¢r©/w d¢ˆ');
 WRITELN(' <left arrow>/<right arrow> -Przesuwanie w tek˜cie');
 WRITELN(' <home>/<end>               -Na pocz¥tek/koniec pliku');
 WRITELN(' <pagr up>/<page down>      -Przesuwa widok o stron© wy¾ej/ni¾ej');
 WRITELN('  <F1>                      -Wy˜wietla ten ekran pomocy');
 WRITELN('  <F2>/<F3>                 -Zmiejszenie/zwi©kszenie ilo˜ci');
 WRITELN('                             znak¢w na jedn¥ lini©');
 WRITELN('  <F4>/<F5>                 -Kopiowanie linii/znaku');
 WRITELN('  <F6>/<F7>                 -Zwi©ksza/zmiejsza wielko˜† parametru [number]');
 WRITELN('        <Return>           -Podanie nowej warto˜ci bajtu.');
END;

PROCEDURE EKRAN(Kod,Atrybut,XPOS,YPOS:Byte);
Var Adres:Word;
Begin
 Adres:=160*(yPOS-1)+(xPOS-1)*2;
 MEM[$b800:ADRES]:=KOD;
 MEM[$B800:ADRES+1]:=ATRYBUT;
END;

Procedure PasekMenu;
Begin
 Gotoxy(1,25);
 textattr:=7;write('F1');textattr:=48;write('Pomoc');
 textattr:=7;write(' F2');textattr:=48;write('Zw©¾ ');
 textattr:=7;write('F3');textattr:=48;write('Poszerz');
 textattr:=7;write('F4');textattr:=48;write('Kop Ln');
 textattr:=7;write('F5');textattr:=48;write('Kop Zn');
 textattr:=7;write('F6');textattr:=48;write('-Bajt');
 textattr:=7;write('F7');textattr:=48;write('+Bajt');
 textattr:=7;write('F8');textattr:=48;write('Znajdz');
 textattr:=7;write(#27,#217);textattr:=48;write('ASCII');
 textattr:=7;write(' F10');textattr:=48;write('Koniec');
End;

Procedure PasekStanu;
Begin
 gotoxy(1,1);
 textattr:=177;
 write(' ':80);
 gotoxy(1,1);
 write('Games Editor: ',name);
 gotoxy(68,1);
 write(n,' zn/ln');
 gotoxy(78,1);
 IF N>80 THEN WRITE('->') ELSE WRITE('  ');
End;

Procedure StanUpdate;
Begin
 textattr:=177;
 gotoxy(36,1);
 write('Bajt ',N*(O+B-2)+P+A:6);
 write(' ASCII ');
 IF N*(O+B-2)+P+A-1<FILESIZE(F1) THEN BEGIN
   SEEK(F1,N*(O+B-2)+P+A-1);READ(F1,Z);write(ORD(Z):3);
 IF NOT EOF(F1) THEN BEGIN READ(F1,Y);C:=ORD(Z)*16;WRITE(' I2:',(C)*16+ORD(Y):5);END;
 End ELSE WRITE('EOF         ');

End;

Procedure WyswietlLinie(NrLini:Byte);
Begin
   {$I-}
   SEEK(F1,(NrLini+o)*N+P);
   If IOResult<>0 then seek(f1,filesize(f1));
   {$i+}
   FOR J:=1 TO GetMaxY DO BEGIN
     If J<=N then Begin IF EOF(F1) THEN X:='=' ELSE READ(F1,X);
                 If (x=#0)and((opcje and 1)>0) then x:=#32;
                 EKRAN(ORD(X),27,J,NrLini+2);
                 End else EKRAN(32,7,J,NrLini+2);
   END;
End;


Procedure Init;
Begin
 IF (PARAMSTR(1)='/?') OR (PARAMSTR(1)='?') THEN BEGIN
   HELPME;
   HALT;
   END;
 WRITELN('Test pocz¥tkowy edytora FX . . .');
 IF PARAMSTR(1)<>'' THEN NAME:=PARAMSTR(1) ELSE BEGIN
   WRITE('Podaj nazw© pliku:');READLN(NAME);END;
 ASSIGN(F1,NAME);
 {$i-}
 RESET(F1);
 if (ioresult<>0) OR (NAME='') then begin
   writeln('Plik nie istnieje !');
   halt;
   end;
 {$i+}
 WRITELN('Plik zostaˆ otwarty.');
 WRITELN;
 IF PARAMSTR(2)<>'' THEN BEGIN
   VAL(PARAMSTR(2),P,I);
   IF I<>0 THEN P:=0;
   END ELSE P:=0;
 N:=60;
 O:=0;
 a:=1;
 b:=2;
 M:=131;
 Opcje:=1;
End;

Procedure Wyswietl;
Begin
 textattr:=7;
 IF (M>0) AND (Not KEYPRESSED) THEN BEGIN
   If (m and 32)>0 then Begin
                        GotoXY(1,24);DelLine;GotoXY(1,2);InsLine;
                        WyswietlLinie(0);
                        End;
   If (m and 64)>0 then Begin
                        GotoXY(1,2);DelLine;GotoXY(1,24);InsLine;
                        WyswietlLinie(22);
                        End;
   If (m and 1)>0 then PasekStanu;
   If (m and 2)>0 then FOR I:=0 TO 22 DO WyswietlLinie(i);
   If (m and 8)>0 then WyswietlLinie(b-2);
   If (m and 16)>0 then FOR I:=b-2 TO 22 DO WyswietlLinie(i);
   If (m and 128)>0 then PasekMenu;
   M:=0;
   END;
 StanUpdate;
 gotoxy(a,b);
 TEXTATTR:=27;
End;

Procedure Done;
Begin
 CLOSE(F1);
 TEXTATTR:=7;
 CLRSCR;
 WRITELN;
 writeln('   Games Editor ver 2.01');
 writeln('      Autor:    T',#111,#109,'as',#122,' ',#76,#105,'s');
 writeln('                            (c) by FX Home Computer Studio 1997-2000');
End;

PROCEDURE SRCHSTR;
VAR NAM2:STRING;
    X:CHAR;
BEGIN
 NAM2:='';
 WRITE(NAM1);
 X:=UPCASE(READKEY);
 IF (X=#0) AND KEYPRESSED THEN BEGIN X:=READKEY;EXIT;END;
 IF X=#27 THEN BEGIN X:=#1;EXIT;END;
 IF X<>#13 THEN REPEAT
   IF (ORD(X)=8) THEN BEGIN
     IF LENGTH(NAM1)>0 THEN BEGIN
       NAM1:=COPY(NAM1,0,LENGTH(NAM1)-1);
       GOTOXY(WHEREX-1,WHEREY);
       WRITE(' ');
       GOTOXY(WHEREX-1,WHEREY);
     END;
   END ELSE BEGIN
     IF LENGTH(NAM1)<42 THEN BEGIN
       WRITE(X);
       NAM1:=NAM1+X;
     END;
   END;
   X:=UPCASE(READKEY);
   IF (X=#0) AND KEYPRESSED THEN BEGIN X:=READKEY;EXIT;END;
   IF X=#27 THEN BEGIN X:=#1;EXIT;END;
 UNTIL (X=CHAR(13));
 {$I-}
 SEEK(F1,N*(O+B-2)+P+A);
 IF IORESULT<>0 THEN EXIT;
 {$I+}
 TextAttr:=48;
 GotoXY(1,WhereY);Write('   Wyszukiwanie . . . ');
 WHILE NOT EOF(F1) DO BEGIN
    READ(F1,X);NAM2:=NAM2+UPCASE(X);
    WHILE LENGTH(NAM2)>LENGTH(NAM1) DO DELETE(NAM2,1,1);
    IF NAM1=NAM2 THEN BEGIN
      A:=FilePos(F1)-P-LENGTH(NAM1)+1;
      O:=(A DIV N);IF O<21 THEN BEGIN B:=O+2;O:=0;END ELSE BEGIN O:=O-20;B:=22;End;
      A:=A MOD N;EXIT;END;END;
END;

Procedure Wykonaj;
Begin
IF X=#0 THEN begin {klawisze strzaˆek i funkcyjne}
   X:=UPCASE(READKEY);
   IF X='K' THEN               {Strzaˆka w lewo}
    IF A>1 THEN A:=A-1 ELSE
     IF B>2  THEN BEGIN
       B:=B-1;A:=N;
       END ELSE
        IF O>0 THEN BEGIN
          M:=32;O:=O-1;A:=N;
          END;
   IF X='M' THEN               {Strzaˆka w prawo}
    IF A<N THEN A:=A+1 ELSE
     IF B<24 THEN BEGIN
        B:=B+1;A:=1; END ELSE
         IF (O*N+P)<FILESIZE(F1) THEN BEGIN
           M:=64;O:=O+1;A:=1;
           END;
   IF X='H' THEN                {do g¢ry}
    IF B>2 THEN B:=B-1 ELSE
     IF O>0 THEN BEGIN
       M:=32;O:=O-1;
       END;
   IF X='P' THEN                {na d¢ˆ}
    IF B<24 THEN B:=B+1 ELSE
     IF ((O+1)*N+P)<FILESIZE(F1) THEN BEGIN
       M:=64;O:=O+1;
       END;
   IF X='=' THEN BEGIN          {F3}
     M:=3; N:=N+1;
     IF O+B-2<A THEN A:=A-(O+B-2) ELSE BEGIN
       I:=(N-1)*(O+B-2)+A; B:=(I DIV N)+2-O; A:=I MOD N;
       IF A>N THEN A:=N;
       WHILE B<2 DO BEGIN O:=O-1;B:=B+1;END;
       END;
     END;
   IF X='<' THEN                {F2}
    IF N>1 THEN BEGIN
     M:=3; N:=N-1;
(*    a:LongInt;	{Pozycja w linii}
    b:LongInt;	{Linia(na ekranie) w ktorej stoi kursor}
    N:LongInt;	{Ilosc znakow w linii}
    O:LongInt;	{Ilosc linii powyzej ekranu}
    P:LongInt;	{Liczba odjetych bajtow}*)
      I:=(N+1)*(O+B-2)+A-1; B:=(I DIV N)+2-O; A:=(I MOD N)+1;
      WHILE B>24 DO BEGIN
        O:=O+1; B:=B-1;
        END;
     END;
   IF X='G' THEN BEGIN          {Home}
     M:=2; O:=0;
     END;
   IF X='O' THEN BEGIN          {End}
     M:=2;
     O:=(FILESIZE(F1)-P) DIV N;
     END;
   IF X='Q' THEN BEGIN          {PgDn}
     M:=2;
     IF (O+23)*N+P<FILESIZE(F1) THEN O:=O+22 ELSE O:=(FILESIZE(F1)-P) DIV N;
     END;
   IF X='I' THEN BEGIN          {PgUp}
     M:=2;
     IF O>23 THEN O:=O-22 ELSE O:=0;
     END;
   IF X=';' THEN BEGIN          {f1}
      M:=131;textattr:=27;clrscr;textattr:=177;
      write('Informacje o programie edytuj¥cym pliki z danymi program¢w                      ');
      textattr:=27;HELPME;writeln;
      textattr:=7;WRITE('                          ');
      textattr:=48;write('--->  Naci˜nij co˜ <---');
      textattr:=7;write('                              ');
      Z:=READKEY;IF Z=#0 THEN Z:=READKEY;
      END;
   IF X='D' THEN X:=#27;        {F10}
   IF (X='>') AND (N*(O+B-2)+P+A+N<FILESIZE(F1)) THEN {F5}
     FOR I:=1 TO N DO BEGIN
        M:=16;
        SEEK(F1,N*(O+B-2)+P+I-1); READ(F1,Y);
        SEEK(F1,N*(O+B-1)+P+I-1); WRITE(F1,Y);
      END;
   IF (X='@') AND (P<FILESIZE(F1)) THEN BEGIN {F7}
     M:=2; P:=P+1;
     IF A>1 THEN A:=A-1 ELSE
      IF B>2 THEN BEGIN
         B:=B-1; A:=N;
       END ELSE
         IF O>0 THEN BEGIN
           O:=O-1;
           A:=N;
          END;
    END;
   IF (X='A') AND (P>0) THEN BEGIN      {F6}
     M:=2;P:=P-1;
     IF A<N THEN A:=A+1 ELSE
      IF B<24 THEN BEGIN
        B:=B+1; A:=1;
       END ELSE
        IF (O*N+P)<FILESIZE(F1) THEN BEGIN
          M:=2; O:=O+1; A:=1;
          END;
    END;
   IF X='B' THEN BEGIN          {F8}
     M:=131;
     GOTOXY(1,25);TEXTATTR:=48;WRITE('Szukaj ˆaäcucha znak¢w:');
     TEXTATTR:=7;WRITE('[');FOR I:=1 TO 42 DO WRITE(' ');WRITE(']');
     GOTOXY(25,25);SRCHSTR;
    END;
   IF (X='?') AND (N*(O+B-2)+P+A+N-1<FILESIZE(F1)) THEN BEGIN
     SEEK(F1,N*(O+B-2)+P+A-1); READ(F1,Y);
     I:=0;
     REPEAT
       SEEK(F1,FILEPOS(F1)+N-1); READ(F1,Z);
       I:=I+1;
       UNTIL (Z<>Y) OR (FILEPOS(F1)+N>FILESIZE(F1));
     if i<24-B then begin
       SEEK(F1,FILEPOS(F1)-1); WRITE(F1,Y);
       M:=16;
      END;
    END;
END {if x=#0} ELSE
 IF (X=#13) AND (N*(O+B-2)+P+A<=FILESIZE(F1)) THEN BEGIN
   M:=129+32+8;
   gotoxy(1,25);TEXTATTR:=48;write('           ');
   TEXTATTR:=7;seek(f1,N*(O+B-2)+P+A-1);read(f1,y);write(ord(y):3);
   TEXTATTR:=48;WRITE('|--->                                                  ');
   TEXTATTR:=7;WRITE(' ESC');GOTOXY(20,25);WRITE('   ');
   GOTOXY(A,B);WRITE(Y);GOTOXY(20,25);
   {$I-}
   READLN(I);
   IF IORESULT=0 THEN BEGIN
     Y:=CHR(I); SEEK(F1,FILEPOS(F1)-1); WRITE(F1,Y);
    END;
   {$I+}
  END ELSE
   IF (X<>#27) THEN BEGIN
     seek(f1,N*(O+B-2)+P+A-1); write(F1,X); WRITE(X);
    END;
End;

BEGIN
 Init;
 REPEAT
  Wyswietl;
  X:=READKEY;
  Wykonaj;
 UNTIL X=#27;
 Done;
END.
