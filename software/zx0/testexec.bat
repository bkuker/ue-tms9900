python ..\..\xdt99\xas99.py -R -b testTarget.a99 -o testTarget.rom -L testTarget.lst
python ..\..\pyZX0\pyzx0.py -f testTarget.rom testTarget.romz
python ..\..\xdt99\xas99.py -R -b testExec.a99 -o testExec.rom -L testExec.lst 