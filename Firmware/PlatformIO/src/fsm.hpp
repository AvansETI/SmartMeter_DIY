// Avans University of Applied Sciences (Avans Hogeschool)
// Module: Intelligent Wireless Sensor Netwerken (IWSN)
// Author: Maurice Snoeren
// Date  : 05-02-2021
// Author: Maurice Snoeren
//
#pragma once

#include <map>
#include <vector>
#include <functional>

#include <Arduino.h>

class FSMEventInterface {
    // Raise an event. When the event is found in the transision matrix, the
    // state machine will go to the next state automatically.
    virtual void raiseEvent(String e) = 0;
};

class FSMState {
    private:
        String name;
        FSMEventInterface* event;        

    public:
        FSMState(String name, FSMEventInterface* event): name(name), event(event) {
        }

        String getName () {
            return this->name;
        }

        // When the state is selected, this is used to initialize the state and is called once.
        virtual int pre() = 0;

        // When the state is active, this function is called in the main loop.
        virtual int loop(unsigned long loopTime) = 0;

        // When another state is selected this is used to perform post functionality before it exists.
        virtual int post() = 0;        
};

class FSMTransition {
    private:
        FSMState* fromState;
        String event;
        FSMState* toState;

    public:
        FSMTransition(): fromState(NULL), event(""), toState(NULL) {
        }

        FSMTransition(FSMState* fromState, String event, FSMState* toState): fromState(fromState), event(event), toState(toState) {
        }

        FSMState* getFromState() {
            return this->fromState;
        }

        FSMState* getToState() {
            return this->toState;
        }

        String getEvent() {
            return this->event;
        }

        void set(FSMState* fromState, String event, FSMState* toState) {
            this->fromState = fromState;
            this->event = event;
            this->toState = toState;
        }
};

/* Finite State Machine class that implements the state machine functionality */
template<unsigned int S, unsigned int T>
class FSM: public FSMEventInterface {
    private:
        // The transaction map holds the transaction. If the transaction is not in the map
        // it does not exist. Usage: newState = map[state][event].
        FSMTransition transitions[T];

        // The states vector contains the three callback methods for each state.
        FSMState* states[S];

        // Total number of states that the state machine contains.
        unsigned int totalStates;

        // Total number of events that can be used with the state machine.
        unsigned int totalTransitions;

        // The current state that the state machine is in.
        FSMState* currentState;

        // The last event that has been raised.
        String lastRaisedEvent;

        // When debugging is enabled, messages are send on the Serial connection.
        bool debugEnabled;

        // To calculate the loop timing, the variable loopTiming is used.
        unsigned long loopTiming;

        // After each loop, the total loop time in milliseconds is calculated and 
        // that is stored in this variable.
        unsigned long currentLoopTime;

    public:
        // Initialized the FSM class.
        FSM(bool debugEnabled = false): totalStates(0), totalTransitions(0), debugEnabled(debugEnabled) {
            this->currentState = NULL;
        }

        // Add a state to the state machine. Make sure the order is correct!
        void addState(FSMState* state) {
            if ( S > this->totalStates ) {
                this->states[this->totalStates] = state;
                this->totalStates++;
                this->_debug("FSM: Added state: " + state->getName());
            } else {
                this->_debug("FSM: Cannot add state: " + state->getName());
            }
        }

        // Adds a new transition to the state machine that will be used to get
        // the next state based on the raised event.
        void addTransition(FSMState* fromState, String event, FSMState* toState) {
            if ( T > this->totalTransitions ) {
                this->transitions[this->totalTransitions].set(fromState, event, toState);
                this->totalTransitions++;
                this->_debug("FSM: Added transition: " + fromState->getName() + " -> " + event + " -> " + toState->getName());
            } else {
                this->_debug("FSM: Cannot add transition: " + fromState->getName() + " -> " + event + " -> " + toState->getName());
            }
        }

        // Raise an event. When the event is found in the transision matrix, the
        // state machine will go to the next state automatically.
        void raiseEvent(String e) {
            this->_debug("FSM: Event raised " + String(e));

            for (unsigned int i = 0; i < this->totalTransitions; i++) {
                FSMTransition* transition = &this->transitions[i];
                if ( this->currentState == transition->getFromState() ) { // We found a transition for the current state
                    if ( e.compareTo(transition->getEvent()) ) { // We found the event to change to!
                        FSMState* newState = transition->getToState();
                        this->_debug("FSM: Changing to new state: " + newState->getName());
                        this->currentState->post(); // Leaving current state call the post()
                        newState->pre(); // Staring new state call the pre();
                        this->currentState = newState; // Set the new state
                    }
                }
            }
        }

        // Starts the FSM
        void start(FSMState* startState) {
            this->currentState = startState;
            this->lastRaisedEvent = "";
            this->currentState->pre(); // Starting the state so call pre()
            this->loopTiming = millis();
            this->_debug("FSM: Started");
        }

        // Call this within the loop of your main application. After the loop
        // an event is raised that has been given by the eventStateExecuted
        // of the setup class.
        void loop() {
            this->currentLoopTime = millis() - this->loopTiming;
            this->loopTiming = millis();

            if ( this->currentState != NULL ) {
                this->currentState->loop(this->loopTiming);

            } else {
                this->_debug("FSM: current state not defined.");
            }

            this->_debug("FSM: current state " + String(this->currentState->getName()));
            this->_debug("FSM: Loop time: " + String(this->currentLoopTime) + "ms");
        }

        // Returns the current loop time in milliseconds.
        unsigned long getLoopTime() {
            return this->currentLoopTime;
        }

    private:
        // Used by the class to provide debug messages in case things go wrong.
        void _debug(String text) {
            if ( this->debugEnabled ) {
               Serial.println(text);
            }
        }

};