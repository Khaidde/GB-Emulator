GB = ./build/gbemu.exe
roms = ./roms

tetris:
	${GB} ${roms}/Tetris.gb

drMario:
	${GB} '${roms}/Dr. Mario (World).gb'

superMarioLand:
	${GB} '${roms}/Super Mario Land (World).gb'

loz:
	${GB} '${roms}/Legend of Zelda, The - Link$'s Awakening (USA, Europe).gb'

bgbtest:
	${GB} C:\Users\berkx\Downloads\bgb\bgbtest.gb