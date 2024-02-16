if ! test -d ../build; then
    mkdir ../build
fi

xcrun -sdk macosx metal -gline-tables-only -MO -g -c ../assets/shaders/Shaders.metal -o ../build/Shaders.air
xcrun -sdk macosx metallib ../build/Shaders.air -o ../build/Shaders.metallib

clang -g -framework Appkit -framework Metal -framework MetalKit -framework GameController -framework AudioToolbox -o ../build/handmade mac_handmade.mm


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


echo Finished!

