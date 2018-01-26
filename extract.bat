for %%f in (*.bnk) do (
bnkextr.exe %%f )
pause

for %%f in (*.txt, *.xml, *.json) do (
xcopy %%f %%~nf )
pause 

for /R %%f in (*.wem) do (
ww2ogg.exe %%f --pcb packed_codebooks_aoTuV_603.bin)
pause