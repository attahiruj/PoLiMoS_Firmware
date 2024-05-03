#define CHILD

#ifdef PARENT
#include "parent_node.h"
#endif	//PARENT

#ifdef CHILD
#include "child_node.h"
#endif	//CHILD

void setup() {
#ifdef PARENT
	parent_setup();
#endif	//PARENT

#ifdef CHILD
	child_setup();
#endif	//CHILD
}

void loop() {
#ifdef PARENT
	parent_update();
#endif	//PARENT

#ifdef CHILD
	child_update();
#endif	//CHILD
}

