#include "bgame.hpp"
#include "logging.h"
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <sys/wait.h>
#include <sys/poll.h>

#define BOMB_EXEC "./bomb"

using namespace std;

void readFile(const char* filename) 
{
	fstream inFile;

	inFile.open(filename, ios::in);

	if (inFile.is_open()) {
		string current_line;
		getline(inFile, current_line);
		stringstream current_stream (current_line);
		current_stream >> mapWidth >> mapHeight >> obsCount >> bomberCount;
		for (unsigned int i = 0; i < obsCount; i++) 
		{
			getline(inFile, current_line);
			current_stream.str(current_line);
			
			ent* newEntity = new ent();
			newEntity->obj_data.type = OBSTACLE;
			current_stream >> newEntity->obj_data.position.x >> newEntity->obj_data.position.y >> newEntity->data;
			entities.push_back(newEntity);
		}
		for (unsigned int i = 0; i < bomberCount; i++) {
			getline(inFile, current_line);
			current_stream.str(current_line);
			int count;
			
			
			ent* newEntity = new ent();
			newEntity->obj_data.type = BOMBER;
			current_stream >> newEntity->obj_data.position.x >> newEntity->obj_data.position.y >> count;
			
			
			char* args[count + 1];
			getline(inFile, current_line);
			istringstream ss(current_line);
			string temp;
			int j = 0;
			while(getline(ss, temp, ' ')){
				args[j] = new char[temp.size()];
				strcpy(args[j], temp.c_str());
				j++;
			}
			args[count] = NULL;
			newEntity->process = new proc(newEntity, args[0], args);
			entities.push_back(newEntity);
			bombers.push_back(newEntity->process);
		}
	}
}

bool recieveMessage(proc* process, im* request){
	if(process->process_id <= 0){
		return false;
	}
	read_data(process->pipe_get[0], request);
	
	imp message;
	message.pid = process->process_id;
	message.m = request;
	print_output(&message, NULL, NULL, NULL);
	return true;
}

void sendMessage(om* output, unsigned int obj_count, od* objects, obsd* obstacle, proc* process, mt type){
	omp message;
	switch (type)
	{
		case REGULAR:
			send_message(process->pipe_send[0], output);

			message.pid = process->process_id;
			message.m = output;
			print_output(NULL, &message, NULL, NULL);
			break;
		
		case VISION:
			send_message(process->pipe_send[0], output);
			if(output->data.object_count > 0)
				send_object_data(process->pipe_send[0], obj_count, objects);
			
			message.pid = process->process_id;
			message.m = output;
			print_output(NULL, &message, NULL, objects);
			break;

		case DAMAGE:
			print_output(NULL, NULL, obstacle, NULL);
			break;
	}
}

void removeFromList(proc* process, ot type){
	if(type == BOMB){
		for(unsigned int i = 0; i < bombs.size(); i++){
			if(isSame(bombs[i], process))
			{
				bombs.erase(bombs.begin() + i);
				return;
			}
		}
	}
	else{
		for(unsigned int i = 0; i < bombers.size(); i++){
			if(isSame(bombers[i], process))
			{
				bombers.erase(bombers.begin() + i);
				return;
			}
		}
	}
}

void removeFromList(ent* entity){
	for(unsigned int i = 0; i < entities.size(); i++){
		if(isSame(entity, entities[i]))
		{
			entities.erase(entities.begin() + i);
			return;
		}
	}
}

void removeEntity(ent* entity){
	ot type = entity->obj_data.type;
	proc* process = entity->process;
	switch (type)
	{
		case OBSTACLE:
			removeFromList(entity);
			delete entity;
			break;
		
		default:
			
			waitpid(process->process_id, NULL, 0);

			close(process->pipe_send[0]);
			close(process->pipe_get[0]);

			removeFromList(entity);
			delete entity;

			removeFromList(process, type);
			delete process;

			break;
	}
}

void damageEntity(ent* entity, unsigned int r) {
	if(entity->obj_data.type == OBSTACLE){
		obsd log;
		log.position = entity->obj_data.position;
		if(entity->data != -1){
			entity->data -= 1;
			log.remaining_durability = entity->data;
			if(entity->data == 0){
				removeEntity(entity);
			}
		}
		else{
			log.remaining_durability = -1;
		}
		sendMessage(NULL, 0, NULL, &log, NULL, DAMAGE);
	}
	else if(entity->obj_data.type == BOMBER){
		entity->status = DEAD;
		entity->data = (int)r;
	}
}

bool isAllDead(){
	for(ent* entity: entities){
		if(entity->obj_data.type == BOMBER && entity->status != DEAD){
			return false;
		}
	}
	return true;
}

vector<unsigned int> getArea(coordinate coord, int range){
	vector<unsigned int> retval;
	unsigned int xp = (unsigned int) min((int) coord.x + range, (int) mapWidth);
	unsigned int yp = (unsigned int) min((int) coord.y + range, (int) mapHeight);
	unsigned int xn = (unsigned int) max((int) coord.x - range, 0);
	unsigned int yn = (unsigned int) max((int) coord.y - range, 0);
	for(ent* entity: entities){
		od temp = entity->obj_data;
		if(temp.position.x == coord.x && temp.position.y >= yn && temp.position.y < coord.y){
			if(temp.type == OBSTACLE){
				yn = temp.position.y;
			}
		}
		else if(temp.position.x == coord.x && temp.position.y <= yp && temp.position.y > coord.y){
			if(temp.type == OBSTACLE){
				yp = temp.position.y;
			}
		}
		else if(temp.position.y == coord.y && temp.position.x >= xn && temp.position.x < coord.x){
			if(temp.type == OBSTACLE){
				xn = temp.position.x;
			}
		}
		else if(temp.position.y == coord.y && temp.position.x <= xp && temp.position.x > coord.x){
			if(temp.type == OBSTACLE){
				xp = temp.position.x;
			}
		}
	}
	
	retval.push_back(xp);
	retval.push_back(xn);
	retval.push_back(yp);
	retval.push_back(yn);
	return retval;
}

void explodeBomb(ent* entity) {
	vector<od> retval;
	vector<unsigned int> ranges = getArea(entity->obj_data.position, entity->data);
	unsigned int xp = ranges[0];
	unsigned int xn = ranges[1];
	unsigned int yp = ranges[2];
	unsigned int yn = ranges[3];
	unsigned int maxR = 0;

	for(ent* e: entities){
		od temp = e->obj_data;
		if(temp.position.x == entity->obj_data.position.x && temp.position.y == entity->obj_data.position.y){
			damageEntity(e, 0);
		}
		else if(temp.position.x == entity->obj_data.position.x && temp.position.y >= yn && temp.position.y < entity->obj_data.position.y){
			damageEntity(e, entity->obj_data.position.y - temp.position.y);
			maxR = max(maxR, entity->obj_data.position.y - temp.position.y);
		}
		else if(temp.position.x == entity->obj_data.position.x && temp.position.y <= yp && temp.position.y > entity->obj_data.position.y){
			damageEntity(e, temp.position.y - entity->obj_data.position.y);
			maxR = max(maxR, temp.position.y - entity->obj_data.position.y);
		}
		else if(temp.position.y == entity->obj_data.position.y && temp.position.x >= xn && temp.position.x < entity->obj_data.position.x){
			damageEntity(e, entity->obj_data.position.x - temp.position.x);
			maxR = max(maxR, entity->obj_data.position.x - temp.position.x);
		}
		else if(temp.position.y == entity->obj_data.position.y && temp.position.x <= xp && temp.position.x > entity->obj_data.position.x){
			damageEntity(e, temp.position.x - entity->obj_data.position.x);
			maxR = max(maxR, temp.position.x - entity->obj_data.position.x);
		}
	}

	if(isAllDead()){
		for(ent* e: entities){
			if(e->obj_data.type == BOMBER && e->data == (int) maxR){
				e->status = WON;
				return;
			}
		}
	}
}

bool canMove(coordinate t_coord, coordinate o_coord){
	if(t_coord.x < 0 || t_coord.x >= mapWidth || t_coord.y < 0 || t_coord.y >= mapHeight){
		return false;
	}
	if(isSame(t_coord, o_coord)){
		return false;
	}
	if(t_coord.x != o_coord.x && t_coord.y != o_coord.y){
		return false;
	}
	if(abs((int)t_coord.x - (int) o_coord.x) == 1 || abs((int)t_coord.y - (int) o_coord.y) == 1){
		for(ent* entity : entities){
			if(entity->obj_data.type != BOMB && entity->obj_data.position.x == t_coord.x && entity->obj_data.position.y == t_coord.y){
				return false;
			}
		}
		return true;
	}
	return false;
}

bool canBomb(coordinate coord){
	for(ent* entity : entities){
		if(entity->obj_data.type == BOMB && entity->obj_data.position.x == coord.x && entity->obj_data.position.y == coord.y){
				return false;
		}
	}
	return true;
}

void createBomb(bd data, coordinate coord){
	ent* temp = new ent();
	temp->obj_data.type = BOMB;
	temp->obj_data.position = coord;
	temp->data = data.radius;
	char* args[3];
	ostringstream oss;
	oss << data.interval; 
	args[0] = BOMB_EXEC;
	args[1] = new char[oss.str().size()];
	strcpy(args[1], oss.str().c_str());
	args[2] = NULL;
	temp->process = new proc(temp, BOMB_EXEC, args);
	entities.push_back(temp);
	bombs.push_back(temp->process);
}

vector<od> getVision(coordinate coord){
	vector<od> retval;
	vector<unsigned int> ranges = getArea(coord, 3);
	unsigned int xp = ranges[0];
	unsigned int xn = ranges[1];
	unsigned int yp = ranges[2];
	unsigned int yn = ranges[3];
	for(ent* entity: entities){
		od temp = entity->obj_data;
		if(temp.position.x == coord.x && temp.position.y == coord.y){
			if(temp.type == BOMB){
				retval.push_back(temp);
			}
		}
		if(temp.position.x == coord.x && temp.position.y >= yn && temp.position.y < coord.y){
			retval.push_back(temp);
		}
		else if(temp.position.x == coord.x && temp.position.y <= yp && temp.position.y > coord.y){
			retval.push_back(temp);
		}
		else if(temp.position.y == coord.y && temp.position.x >= xn && temp.position.x < coord.x){
			retval.push_back(temp);
		}
		else if(temp.position.y == coord.y && temp.position.x <= xp && temp.position.x > coord.x){
			retval.push_back(temp);
		}
	}
	return retval;

}

int main(int argc, char* argv[])
{
	readFile(argv[1]);

	usleep(15);

	while(true){
		struct pollfd bombpolls[bombs.size() * 2];
		struct pollfd bomberpolls[bombers.size() * 2];

		for(unsigned int i = 0; i < bombs.size(); i++){
			bombpolls[2 * i].fd = bombs[i]->pipe_get[0];
			bombpolls[2 * i].events = POLLIN;
			bombpolls[2 * i + 1].fd = bombs[i]-> pipe_send[0];
			bombpolls[2 * i + 1].events = POLLOUT;
		}
		for(unsigned int i = 0; i < bombers.size(); i++){
			bomberpolls[2 * i].fd = bombers[i]->pipe_get[0];
			bomberpolls[2 * i].events = POLLIN;
			bomberpolls[2 * i + 1].fd = bombers[i]->pipe_send[0];
			bomberpolls[2 * i + 1].events = POLLOUT;
		}
		int result = 0;
		result += poll(bombpolls, 2 *  bombs.size(), 5);
		result += poll(bomberpolls, 2 * bombers.size(), 5);

		if(result > 0){
			vector<proc*> polled;
			for(unsigned int i = 0; i < bombs.size(); i++){
				if(bombpolls[2 * i].revents & POLLIN){
					polled.push_back(bombs[i]);
				}
			}
			for(unsigned int i = 0; i < bombers.size(); i++){
				if(bomberpolls[2 * i].revents & POLLIN && !(bomberpolls[2 * i].revents & POLLHUP) && bomberpolls[2 * i + 1].revents & POLLOUT ){
					polled.push_back(bombers[i]);
				}
			}
			for(proc* process : polled){
				om output;
				im request;
				if(recieveMessage(process, &request)){
					
					if(process->base_entity->status == DEAD){
						output.type = BOMBER_DIE;
						sendMessage(&output, 0, NULL, NULL, process, REGULAR);
						removeEntity(process->base_entity);
					}
					else if(process->base_entity->status == WON){
						output.type = BOMBER_WIN;
						sendMessage(&output, 0, NULL, NULL, process, REGULAR);
						removeEntity(process->base_entity);
					}
					else
					{
						vector<od> temp;
						switch (request.type){
							case BOMBER_START:
								output.type = BOMBER_LOCATION;
								output.data.new_position = process->base_entity->obj_data.position;
								sendMessage(&output, 0, NULL, NULL, process, REGULAR);
								break;
							case BOMBER_SEE:
								temp = getVision(process->base_entity->obj_data.position);
								output.type = BOMBER_VISION;
								output.data.object_count = temp.size();
								sendMessage(&output, temp.size(), &temp[0], NULL, process, VISION);
								break;
							case BOMBER_MOVE:
								if(canMove(request.data.target_position, process->base_entity->obj_data.position)){
									process->base_entity->obj_data.position = request.data.target_position;
								}
								output.type = BOMBER_LOCATION;
								output.data.new_position = process->base_entity->obj_data.position;
								sendMessage(&output, 0, NULL, NULL, process, REGULAR);
								break;
							case BOMBER_PLANT:
								output.type = BOMBER_PLANT_RESULT;
								if(canBomb(process->base_entity->obj_data.position)){
									createBomb(request.data.bomb_info, process->base_entity->obj_data.position);
									output.data.planted = 1;
								}
								else{
									output.data.planted = 0;
								}
								sendMessage(&output, 0, NULL, NULL, process, REGULAR);
								break;
							case BOMB_EXPLODE:
								explodeBomb(process->base_entity);
								removeEntity(process->base_entity);
								break;
						}
					}
				}
			}

		}
		if(bombers.size() == 1 && bombers[0]->base_entity->status == ALIVE){
			bombers[0]->base_entity->status = WON;
		}
		if(bombers.size() == 0 && bombs.size() == 0){
			break;
		}
		usleep(1000);
		
	}

	return 0;
}