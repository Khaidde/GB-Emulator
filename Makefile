GB = ./build/gbemu.exe
roms = ./roms

tetris:
	${GB} ${roms}/Tetris.gb

bgbtest:
	${GB} C:\Users\berkx\Downloads\bgb\bgbtest.gb