#include "../../dionetlib.h"
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

// Networking constants
#define ADDRESS "127.0.0.1"
#define PORT "8888"

#define MAX_CLIENTS 30
#define BUFFER_SIZE 256

// Quick trick to avoid undefined reference to WinMain@16 at compilation with SDL2
#undef main

// Window size (in pixels)
#define WIN_W 800
#define WIN_H 800

#define MAX_FPS 60.f
const Uint32 FPS_CAP=(Uint32)(1000.f/(MAX_FPS+10.f));

// Structure to represent a player's data
struct player{
	int8_t id; // if player doesnt exist, id=-1
	int16_t x; // position of the player
	int16_t y;
};
struct player players[MAX_CLIENTS];
SDL_Point player_positions[MAX_CLIENTS]; // target positions of players so we can smoothe their movements with lerp()
int player_id=-1; // Needs to be set when we join a server
bool ignore_self_updates=false; // if true, this player will ignore messages that update its own info (to make things smoother)

// The map for parkoor and other shenanigans
SDL_Rect default_map[]={
	(SDL_Rect){0,500,800,10}
};

// Package types
#define PKG_PLAYER_UPDATE 0x01 // uint8_t pkg_type+(struct player player_info) -> 6 bytes  (if player didn't exist, player now exists)
#define PKG_PLAYER_QUIT 0x02   // uint8_t pkg_type+(uint8_t id) -> 1 byte (sets the player's id to -1)
#define PKG_PLAYER_SETID 0x03  // uint8_t pkg_type+(uint8_t id) -> 2 bytes (sends the player it's assigned id)

// buffer used for data sent/received
char buffer[BUFFER_SIZE];

// SDL related variables
SDL_Window* win;
SDL_Renderer* rend;
SDL_Event event;

// Assets
SDL_Texture* player_texture=NULL;
SDL_Texture* fps_counter_texture=NULL;
SDL_Rect fps_counter_rect=(SDL_Rect){0,0,0,0};
TTF_Font* terminal_font=NULL;

// Linear Interpolation formula
#define lerp(a,b,t) ((a)+((b)-(a))*(t))
int lerp_i(int a, int b, float t) { return (int)(lerp((float)a,(float)b,t)); } // lerp but with integers
#define abs(x) ((x)>0?(x):-(x))
#define sign(x) ((x)<0?-1:((x)>0?1:0))

// Initialise everything SDL needs
bool init(){
	// Initialise all SDL libraries
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0){
		printf("SDL_Init() error: %s\n",SDL_GetError());
		return false;
	}if(IMG_Init(IMG_INIT_PNG) < 0){
		printf("IMG_Init() error: %s\n",IMG_GetError());
		return false;
	}if(TTF_Init() < 0){
		printf("TTF_Init() error: %s\n",TTF_GetError());
		return false;
	}
	// Create window and renderer
	win=SDL_CreateWindow("sample - Multiplayer game",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,WIN_W,WIN_H,SDL_WINDOW_SHOWN);
	if(win == NULL) { printf("SDL_CreateWindow() error: %s\n",SDL_GetError()); return false; }
	rend=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED);
	if(rend == NULL) { printf("SDL_CreateRenderer() error: %s\n",SDL_GetError()); return false; }
	return true;
}

SDL_Texture* load_texture(char* path){
	SDL_Surface* loaded_surface=IMG_Load(path);
	if(loaded_surface == NULL) { printf("IMG_Load() error: %s\n",IMG_GetError()); return NULL; }
	SDL_Texture* loaded_texture=SDL_CreateTextureFromSurface(rend,loaded_surface);
	SDL_FreeSurface(loaded_surface);
	if(loaded_texture == NULL) printf("SDL_CreateTextureFromSurface() error: %s\n",SDL_GetError());
	return loaded_texture;
}
SDL_Texture* load_text(char* text, SDL_Color color, TTF_Font* font){
	SDL_Surface* loaded_surface=TTF_RenderText_Solid(font,text,color);
	if(loaded_surface == NULL) { printf("TTF_RenderText_Solid() error: %s\n",TTF_GetError()); return NULL; }
	SDL_Texture* loaded_texture=SDL_CreateTextureFromSurface(rend,loaded_surface);
	SDL_FreeSurface(loaded_surface);
	if(loaded_texture == NULL) printf("SDL_CreateTextureFromSurface() error: %s\n",SDL_GetError());
	return loaded_texture;
}

void load_assets(){
	player_texture=load_texture("assets/player.png");
	if(player_texture != NULL) printf("assets/player.png loaded!\n");
	terminal_font=TTF_OpenFont("assets/terminal.ttf",25);
	if(terminal_font != NULL) printf("assets/terminal.ttf loaded!\n");
}

void free_assets(){
	SDL_DestroyTexture(player_texture);
	if(fps_counter_texture != NULL) SDL_DestroyTexture(fps_counter_texture);
	TTF_CloseFont(terminal_font);
}

// Free up resources
void cleanup(){
	free_assets();
	SDL_DestroyRenderer(rend);
	rend=NULL;
	SDL_DestroyWindow(win);
	win=NULL;
	TTF_Quit(); // Quit libraries
	IMG_Quit();
	SDL_Quit();
	// exit with s_quit if dionetlib still not quit, otherwise just exit()
	if(s_active) s_quit();
	else exit(0);
}

// Returns if packet received, sets buffer to packet received if true
bool get_packet(s_socket client_socket){
	bzero(buffer,BUFFER_SIZE);
	s_recv_silent(client_socket,buffer,BUFFER_SIZE-1);
	if(s_recv_result < 0){
		if(s_errno == s_EWOULDBLOCK || s_errno == s_EAGAIN)
			return true;
		perror("recv() error!\n");
		return false;
	}else if(s_recv_result == 0){
		printf("server disconnected!\n");
		return false;
	}
	return true;
}

/*void print_players(){
	for(int i=0; i<MAX_CLIENTS; i++){
		if(players[i].id != -1)
			printf(", {%d,%d,%d}",players[i].id,players[i].x,players[i].y);
	}printf("\n");
}*/

// Will parse through packets and interpret their data
void handle_packet(int data_size){
	int buffer_pos=0;
	while(buffer_pos < data_size){
		switch(buffer[buffer_pos]){
			case (char)PKG_PLAYER_QUIT:{
				int pl_id=(int)buffer[++buffer_pos];
				players[pl_id]=(struct player){-1,0,0};
				player_positions[pl_id]=(SDL_Point){0,0};
				printf("player #%d quit!\n",pl_id);
				buffer_pos++;
				break;
			}case (char)PKG_PLAYER_UPDATE:{
				struct player player_info;
				memcpy(&player_info,&buffer[++buffer_pos],sizeof(struct player));
				if(player_info.id != player_id || !ignore_self_updates){
					if(player_info.id == player_id) {
						ignore_self_updates=true;
						players[player_info.id].x=player_info.x;
						players[player_info.id].y=player_info.y;
					}
					// get the updated position and set it as the target position for lerp_i
					player_positions[player_info.id]=(SDL_Point){(int)player_info.x,(int)player_info.y};
					players[player_info.id].id=player_info.id;
				}buffer_pos+=sizeof(struct player);
				break;
			}case (char)PKG_PLAYER_SETID:
				player_id=(int)buffer[++buffer_pos];
				printf("player id set to %d!\n",player_id);
				buffer_pos++;
				break;
			default:
				printf("invalid packet received!\n");
				buffer_pos++;
		}
	}
}

// Render players
void render_players(){
	for(int i=0; i<MAX_CLIENTS; i++){
		if(players[i].id != -1){ // only render when player exists, duh
			if(i != player_id){
				players[i].x=lerp_i(players[i].x,player_positions[i].x,0.75f); // update the position by using linear interpolation for other players
				players[i].y=lerp_i(players[i].y,player_positions[i].y,0.75f);
			}
			SDL_Rect pl_rect=(SDL_Rect){players[i].x-32-players[player_id].x+WIN_W/2, players[i].y-32-players[player_id].y+WIN_H/2, 64, 64};
			SDL_RenderCopy(rend,player_texture,NULL,&pl_rect);
		}
	}
}

// Render each rectangle in default_map
void render_map(){
	for(int i=0; i<sizeof(default_map)/sizeof(SDL_Rect); i++){
		SDL_SetRenderDrawColor(rend,51,102,0,255);
		SDL_Rect mod_rect=default_map[i];
		mod_rect.x-=mod_rect.w/2+players[player_id].x-WIN_W/2;
		mod_rect.y-=mod_rect.h/2+players[player_id].y-WIN_H/2;
		SDL_RenderFillRect(rend,&mod_rect);
	}
}

// Test collision with all map elements and players
void test_collision(int* delta_x, float* delta_y){
	SDL_Point pl_rect=(SDL_Point){players[player_id].x, players[player_id].y}; // Fake rect (too lazy to change the code)
	// Test map elements
	for(int i=0; i<sizeof(default_map)/sizeof(SDL_Rect); i++){
		int x_diff=default_map[i].x-pl_rect.x, y_diff=default_map[i].y-pl_rect.y;
		if(abs(x_diff) < 32+default_map[i].w/2 && abs(y_diff) < 32+default_map[i].h/2){
			int x_result=(32+default_map[i].w/2-abs(x_diff))*-sign(x_diff);
			float y_result=(float)((32+default_map[i].h/2-abs(y_diff))*-sign(y_diff));
			if(delta_x != NULL){
				*delta_x=abs(x_result)>abs(*delta_x)?x_result:*delta_x;
			}
			if(delta_y != NULL){
				*delta_y=abs(y_result)>abs(*delta_y)?y_result:*delta_y;
			}
		}
	}
	// Test other players
	for(int i=0; i<MAX_CLIENTS; i++){
		if(i == player_id || players[i].id == -1) continue;
		SDL_Point other_rect=player_positions[i];
		int x_diff=other_rect.x-pl_rect.x, y_diff=other_rect.y-pl_rect.y;
		if(abs(x_diff) < 64 && abs(y_diff) < 64){
			int x_result=(64-abs(x_diff))*-sign(x_diff);
			float y_result=(float)((64-abs(y_diff))*-sign(y_diff));
			if(delta_x != NULL){
				*delta_x=abs(x_result)>abs(*delta_x)?x_result:*delta_x;
				printf("coll x: %d\n",*delta_x);
			}
			if(delta_y != NULL){
				*delta_y=abs(y_result)>abs(*delta_y)?y_result:*delta_y;
				printf("coll y: %.0f\n",*delta_y);
			}
		}
	}
}

int main(int argc, char* argv[]){
	// Connect to server and quit if not able to connect
	s_init();
	create_client_socket(ADDRESS,PORT);
	connect_client_socket;
	if(!s_success) { printf("connection error!\n"); s_quit(); }
	// set socket to non-blocking mode because we want to be able to handle game logic/graphics even when packets are not received
	set_socket_block(client_socket,false);
	if(!s_success) { printf("socket block error!\n"); s_quit(); }
	
	// Set all players to default
	for(int i=0; i<MAX_CLIENTS; i++) { players[i]=(struct player){-1,0,0}; player_positions[i]=(SDL_Point){0,0}; }
	
	// Init everything with SDL2
	if(!init()) return -1;
	
	load_assets();
	
	int left_vel=0, right_vel=0, x_vel=0;
	float y_vel=0.f;
	bool jumping=false, on_land=false;
	
	Uint32 LAST_TICK=0;
	float FPS=0.d;
	
	// Game loop
	while(true){
		Uint32 NOW_TICK=SDL_GetTicks();
		if(NOW_TICK-LAST_TICK >= FPS_CAP){
			FPS=1.f/((float)(NOW_TICK-LAST_TICK) / 1000.f);
			char fps_counter_string[25]; sprintf(fps_counter_string,"FPS: %.1f",FPS);
			if(fps_counter_texture != NULL) SDL_DestroyTexture(fps_counter_texture);
			fps_counter_texture=load_text(fps_counter_string,(SDL_Color){255,255,255,255},terminal_font);
			SDL_QueryTexture(fps_counter_texture,NULL,NULL,&fps_counter_rect.w,&fps_counter_rect.h);
			LAST_TICK=NOW_TICK;
		}else
			SDL_Delay(LAST_TICK+FPS_CAP-NOW_TICK);
		
		// Handle SDL2 events
		while(SDL_PollEvent(&event) != 0){
			if(event.type == SDL_QUIT)
				cleanup();
			if(player_id == -1) continue;
			if(event.type == SDL_KEYDOWN){
				switch(event.key.keysym.sym){
					case SDLK_SPACE:
						jumping=true;
						break;
					case SDLK_w:
						jumping=true;
						break;
					case SDLK_a:
						left_vel=1;
						break;
					case SDLK_d:
						right_vel=1;
						break;
				}
			}else if(event.type == SDL_KEYUP){
				switch(event.key.keysym.sym){
					case SDLK_SPACE:
						jumping=false;
						break;
					case SDLK_w:
						jumping=false;
						break;
					case SDLK_a:
						left_vel=0;
						break;
					case SDLK_d:
						right_vel=0;
						break;
				}
			}
		}
		
		// Get newest packet if available
		if(!get_packet(client_socket)){
			closesocket(client_socket);
			cleanup();
		}else{
			handle_packet(s_recv_result);
		}
		
		// Skip this if we still havent received our player id
		if(player_id != -1){
			x_vel=(right_vel-left_vel)*4; // 4 is our speed multiplier
			// Apply gravity to the player
			y_vel += 0.098f;
			// Jump if we are on ground
			if(jumping && on_land){
				y_vel=-6.5f;
				on_land=false;
			}
			// Y-axis collision detection
			float delta_y=0.f;
			players[player_id].y += (int)y_vel;
			test_collision(NULL,&delta_y);
			if(delta_y != 0.f){
				players[player_id].y += (int)delta_y;
				if(y_vel > 0) on_land=true;
				y_vel=0.f;
			}else on_land=false;
			// X-axis collision detection
			int delta_x=0;
			players[player_id].x += x_vel;
			test_collision(&delta_x,NULL);
			if(delta_x != 0)
				players[player_id].x += delta_x;
			// If our position has changed, send an update to the server
			if(x_vel != 0 || (int)y_vel != 0 || delta_x != 0 || (int)delta_y != 0){ // send updated position to server (or at least try)
				bzero(buffer,BUFFER_SIZE);
				buffer[0]=(char)PKG_PLAYER_UPDATE;
				memcpy(buffer+1,&players[player_id],sizeof(struct player));
				s_send_silent(client_socket,buffer,sizeof(struct player)+1);
				if(!s_success && s_errno != s_EAGAIN && s_errno != s_EWOULDBLOCK)
					fprintf(stderr,"send() error: %d\n",s_errno);
			}
		}
		
		SDL_SetRenderDrawColor(rend,0,0,0,255);
		SDL_RenderClear(rend);
		render_map();
		render_players();
		SDL_RenderCopy(rend,fps_counter_texture,NULL,&fps_counter_rect);
		SDL_RenderPresent(rend);
	}
	
	cleanup();
	return 0;
}