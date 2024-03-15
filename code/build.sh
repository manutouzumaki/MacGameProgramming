if ! test -d ../build; then
    mkdir ../build
fi

xcrun -sdk macosx metal -gline-tables-only -MO -g -c ../assets/shaders/Shaders.metal -o ../build/Shaders.air
xcrun -sdk macosx metallib ../build/Shaders.air -o ../build/Shaders.metallib

clang -g -O0 -DHANDMADE_DEBUG  -lstdc++  -framework Appkit -framework Metal -framework MetalKit -framework GameController -framework AudioToolbox -o ../build/client mac_client_main.mm


echo Client compiled

echo Building App

if ! test -d ../build/client.app; then
    mkdir ../build/client.app
fi

if ! test -d ../build/resources; then
    mkdir ../build/resources
fi

rm -rf ../build/client.app
mkdir -p ../build/client.app/Contents/Resources

cp ../build/client ../build/client.app/client
cp ../build/resources/Info.plist ../build/client.app/Contents/Info.plist
cp ../build/Shaders.metallib ../build/client.app/Contents/Resources/Shaders.metallib
cp ../assets/sounds/test.wav ../build/client.app/Contents/Resources/test.wav
cp ../assets/sounds/test1.wav ../build/client.app/Contents/Resources/test1.wav
cp ../assets/textures/link.png ../build/client.app/Contents/Resources/link.png
cp ../assets/textures/grass.png ../build/client.app/Contents/Resources/grass.png
cp ../assets/textures/tilemap.png ../build/client.app/Contents/Resources/tilemap.png
cp ../assets/tilemaps/tilemap.csv ../build/client.app/Contents/Resources/tilemap.csv
cp ../assets/tilemaps/collision.csv ../build/client.app/Contents/Resources/collision.csv

clang -g -O0 -DHANDMADE_DEBUG -lstdc++ -std=c++11 -o ../build/server server_main.cpp

echo Server compiled

echo Finished!

