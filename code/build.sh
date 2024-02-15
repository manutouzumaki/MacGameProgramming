if ! test -d ../build; then
    mkdir ../build
fi

xcrun -sdk macosx metal -gline-tables-only -MO -g -c ../assets/shaders/Shaders.metal -o ../build/Shaders.air
xcrun -sdk macosx metallib ../build/Shaders.air -o ../build/Shaders.metallib

clang -O0 -g -framework Appkit -framework Metal -framework MetalKit -framework GameController -framework AudioToolbox -o ../build/handmade handmade_mac.mm


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


echo Finished!
