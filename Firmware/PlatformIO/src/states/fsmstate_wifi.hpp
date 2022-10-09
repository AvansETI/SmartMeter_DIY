#include <fsm.hpp>

class FSMState_WiFi: public FSMState {
    private:

    public:
        FSMState_WiFi(FSMEventInterface* event): FSMState("START", event) {
        }

        void setup() {

        }


        // When the state is selected, this is used to initialize the state and is called once.
        int pre() {
            return 0;
        }

        // When the state is active, this function is called in the main loop.
        int loop(unsigned long loopTime) {
            return 0;
        }

        // When another state is selected this is used to perform post functionality before it exists.
        int post() {
            return 0;
        } 
};