call make clean
call make
python ..\..\..\pyZX0\pyzx0.py -f forth.rom forth.romz
python ..\..\..\xdt99\xas99.py -R -b boot.a99 -o forthBoot.rom -L forthBoot.lst
copy forthBoot.* ..\..\web\public