#include "message.h"
#include <unistd.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <iostream>


struct ent;
int maxDesc = 0;

typedef enum bomber_status{
	ALIVE,
	DEAD,
	WON
}bs;

typedef enum message_type{
	REGULAR,
	VISION,
	DAMAGE
}mt;

typedef struct proc {
	pid_t process_id;
	int pipe_send[2];
	int pipe_get[2];
	ent* base_entity;
	proc(ent* e, const char* path, char* args[]) {
		base_entity = e;
		PIPE(pipe_send);
		PIPE(pipe_get);
		process_id = fork();
		if (process_id == 0) {
			dup2(pipe_send[1], fileno(stdin));
			dup2(pipe_get[1], fileno(stdout));
			close(pipe_get[0]);
			close(pipe_send[0]);
			execv(path, args);
		}
		else{
			close(pipe_get[1]);
			close(pipe_send[1]);
		}
	};
}proc;

typedef struct ent {
	proc* process;
	od obj_data;
	int data;
	bomber_status status = ALIVE;
}ent;

std::vector<ent*> entities;
std::vector<proc*> bombers;
std::vector<proc*> bombs;
unsigned int mapWidth;
unsigned int mapHeight;
unsigned int obsCount;
unsigned int bomberCount;


bool isSame(coordinate coord1, coordinate coord2){ return coord1.x == coord2.x && coord1.y == coord2.y;}

bool isSame(proc* p1, proc* p2){return p1->process_id == p2->process_id;}

bool isSame(ent* e1, ent* e2){
	if(e1->obj_data.type != e2->obj_data.type){
		return false;
	}
	if(e1->obj_data.type == OBSTACLE){
		return isSame(e1->obj_data.position, e2->obj_data.position);
	}
	return isSame(e1->process, e2->process);
}