//
//  mac_input.cpp
//
//
//  Created by Manuel Cabrerizo on 16/02/2024.
//

const uint32 MAC_MAX_CONTROLLER_COUNT = 4;
struct MacInput {
    GCController *controllers[MAC_MAX_CONTROLLER_COUNT];
    uint32 controllerConected;
    GCKeyboard *keyboard;

    GameInput inputs[2];
    GameInput *currentInput;
    GameInput *lastInput;
};

void MacProcessInput(MacInput *macInput, GameInput *input) {
    
    for(uint32  i = 0; i < MAC_MAX_CONTROLLER_COUNT; i++) {
        for(int32 j = 0; j < 8; j++) {
            input->controllers[i].buttons[j].endedDown = false;
        }
        
        GCController *controller = macInput->controllers[i];
        if(controller) {
            GCExtendedGamepad *gamepad = [controller extendedGamepad];
            input->controllers[i].A.endedDown = gamepad.buttonA.isPressed;
            input->controllers[i].B.endedDown = gamepad.buttonB.isPressed;
            input->controllers[i].X.endedDown = gamepad.buttonX.isPressed;
            input->controllers[i].Y.endedDown = gamepad.buttonY.isPressed;
            input->controllers[i].left.endedDown = gamepad.dpad.left.isPressed;
            input->controllers[i].right.endedDown = gamepad.dpad.right.isPressed;
            input->controllers[i].up.endedDown = gamepad.dpad.up.isPressed;
            input->controllers[i].down.endedDown = gamepad.dpad.down.isPressed;
        }
    }
    
    GCKeyboardInput *keyboardInput = [macInput->keyboard keyboardInput];
    input->controllers[0].A.endedDown |= [keyboardInput buttonForKeyCode: GCKeyCodeSpacebar].pressed;
    input->controllers[0].B.endedDown |= [keyboardInput buttonForKeyCode: GCKeyCodeKeyB].pressed;
    input->controllers[0].X.endedDown |= [keyboardInput buttonForKeyCode: GCKeyCodeKeyR].pressed;
    input->controllers[0].Y.endedDown |= [keyboardInput buttonForKeyCode: GCKeyCodeKeyF].pressed;
    
    input->controllers[0].left.endedDown |= [keyboardInput buttonForKeyCode: GCKeyCodeLeftArrow].pressed;
    input->controllers[0].right.endedDown |= [keyboardInput buttonForKeyCode: GCKeyCodeRightArrow].pressed;
    input->controllers[0].up.endedDown |= [keyboardInput buttonForKeyCode: GCKeyCodeUpArrow].pressed;
    input->controllers[0].down.endedDown |= [keyboardInput buttonForKeyCode: GCKeyCodeDownArrow].pressed;
    
}
