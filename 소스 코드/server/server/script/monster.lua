myid = 9999999;
playerid = 9999999;
state = 0;
n_moved = 0;

function set_myid(x)
    myid = x;
	math.randomseed(os.time());
end

function event_player_move(player)
	if (state == 0) then
		playerid = player;
		player_x = API_get_x(player);
		player_y = API_get_y(player);
		my_x = API_get_x(myid);
		my_y = API_get_y(myid);

		if (player_x == my_x) then
			if(player_y == my_y) then 
				API_send_message(player, myid, "HELLOO");
				dir = math.random(4) - 1;
				API_set_npc_dir(dir, myid);
				API_register_event(1000, 2, myid);
				state = 1;
			end
		end
	end
end

function event_move()
	if(state == 1) then
		API_move_npc(myid);
		n_moved = n_moved + 1;
		if (n_moved == 3) then
			state = 0;
			n_moved = 0;
			API_send_message(playerid, myid, "BYEE");
		else
			API_register_event(1000, 2, myid);
		end
	end
end