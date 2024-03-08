if ! test -d ../build; then
    mkdir ../build
fi

xcrun -sdk macosx metal -gline-tables-only -MO -g -c ../assets/shaders/Shaders.metal -o ../build/Shaders.air
xcrun -sdk macosx metallib ../build/Shaders.air -o ../build/Shaders.metallib

clang -g -O0 -DHANDMADE_DEBUG  -lstdc++  -framework Appkit -framework Metal -framework MetalKit -framework GameController -framework AudioToolbox -o ../build/handmade mac_handmade.mm


echo HandmadeMac compiled

echo Building App

if ! test -d ../build/handmade.app; then
    mkdir ../build/handmade.app
fi

if ! test -d ../build/resources; then
    mkdir ../build/resources
fi

rm -rf ../build/handmade.app
mkdir -p ../build/handmade.app/Contents/Resources

cp ../build/handmade ../build/handmade.app/handmade
cp ../build/resources/Info.plist ../build/handmade.app/Contents/Info.plist
cp ../build/Shaders.metallib ../build/handmade.app/Contents/Resources/Shaders.metallib
cp ../assets/sounds/test.wav ../build/handmade.app/Contents/Resources/test.wav
cp ../assets/sounds/test1.wav ../build/handmade.app/Contents/Resources/test1.wav
cp ../assets/textures/link.png ../build/handmade.app/Contents/Resources/link.png
cp ../assets/textures/grass.png ../build/handmade.app/Contents/Resources/grass.png
cp ../assets/textures/tilemap.png ../build/handmade.app/Contents/Resources/tilemap.png
cp ../assets/tilemaps/tilemap.csv ../build/handmade.app/Contents/Resources/tilemap.csv
cp ../assets/tilemaps/collision.csv ../build/handmade.app/Contents/Resources/collision.csv


echo Finished!

