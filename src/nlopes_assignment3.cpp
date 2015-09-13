/**
 * @nlopes_assignment3
 * @author  Nikhil Lopes <nlopes@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <sys/time.h>
#include <fstream>

#include "../include/global.h"
#include "../include/logger.h"

using namespace std;
#define inf 65535;
#define btoa(x) ((x)?"true":"false")


//[PA3] Update Packet Start
struct entry
{
	struct in_addr ip_address;
	uint16_t port;
	uint16_t padding;
	uint16_t server_id;
	uint16_t cost;
};

struct packet
{
	uint16_t update_fields;
	uint16_t port;
	struct in_addr ip_address;
	struct entry entries[];
};
//[PA3] Update Packet End

//[PA3] Routing Table Start
struct routing_table
{
	uint16_t server_id;
	struct in_addr ip_address;										//struct from beef
	uint16_t port;
	int next_hop;
	uint16_t cost;

	bool isNeighbour;
	bool isEnabled;
	bool isPresent;
	int time_since_update;
};
//[PA3] Routing Table End

void print_usage()
{
	cout<<("Usage: ./assignment3 -t<pathtotopologyfile> -i<routingupdateinterval>")<<endl;
	cout<<"pathtotopologyfile: The topology file contains the initial topology configuration for the server"<<endl;
	cout<<"routingupdateinterval: It specifies the time interval between routing updates in seconds."<<endl;
}

int send_update(int socket, int my_server_id, int number_servers, struct routing_table *local_table)											//send update
{
	ssize_t bytes_sent,size=(2*sizeof(uint16_t))+(sizeof(struct in_addr))+(number_servers*sizeof(struct entry));
	struct packet *local_packet=(packet*)malloc(size);
	local_packet->update_fields=htons(number_servers);
	local_packet->port=htons(local_table[my_server_id].port);
	local_packet->ip_address.s_addr=local_table[my_server_id].ip_address.s_addr;
	for(int i=1;i<=number_servers;i++)
	{
		local_packet->entries[i-1].ip_address.s_addr=local_table[i].ip_address.s_addr;
		local_packet->entries[i-1].port=htons(local_table[i].port);
		local_packet->entries[i-1].padding=htons(0x0);
		local_packet->entries[i-1].server_id=htons(local_table[i].server_id);
		local_packet->entries[i-1].cost=htons(local_table[i].cost);
	}
	for(int i=1;i<=number_servers;i++)													//send to all neighbours?
	{
		if(local_table[i].isNeighbour==true && local_table[i].isEnabled==true)
		{
			struct sockaddr_in dest;
			dest.sin_family=AF_INET;
			dest.sin_port=htons(local_table[i].port);
			dest.sin_addr=local_table[i].ip_address;
			bytes_sent=sendto(socket, (struct packet*)local_packet, size, 0,(struct sockaddr*)&dest, sizeof dest);
		}
	}
	return 0;
}
int dump(int my_server_id, int number_servers, struct routing_table *local_table)
{
	ssize_t size=(2*sizeof(uint16_t))+(sizeof(struct in_addr))+(number_servers*sizeof(struct entry));
	struct packet *local_packet=(packet*)malloc(size);
	local_packet->update_fields=htons(number_servers);
	local_packet->port=htons(local_table[my_server_id].port);
	local_packet->ip_address.s_addr=local_table[my_server_id].ip_address.s_addr;
	for(int i=1;i<=number_servers;i++)
	{
		local_packet->entries[i-1].ip_address.s_addr=local_table[i].ip_address.s_addr;
		local_packet->entries[i-1].port=htons(local_table[i].port);
		local_packet->entries[i-1].padding=htons(0x0);
		local_packet->entries[i-1].server_id=htons(local_table[i].server_id);
		local_packet->entries[i-1].cost=htons(local_table[i].cost);
	}
	if (cse4589_dump_packet((struct packet*)local_packet, size)==size)							// how should ip address appear
	{
		return 0;
	}
	else
	{
		return -1;
	}
}
void state(int my_server_id, int number_servers, struct routing_table *local_table, uint16_t distance_vector[][10])				//for my debugging, prints out local data and dv
{

	cout<<"Local_Table"<<endl;
	printf("%-15s%-15s%-15s%-15s%-15s%-15s%-15s%-15s%-15s\n", "Id","IP Address","Port","isNeighbour","isEnabled","isPresent","next_hop","cost","time");
	for(int i=1; i<=number_servers; i++)
	{
		char *str=(char*)malloc(INET_ADDRSTRLEN);
		inet_ntop(AF_INET, &(local_table[i].ip_address.s_addr), str, INET_ADDRSTRLEN);
		printf("%-15d%-15s%-15d%-15s%-15s%-15s%-15d%-15d%-15d\n", local_table[i].server_id,str,local_table[i].port,btoa(local_table[i].isNeighbour),btoa(local_table[i].isEnabled),btoa(local_table[i].isPresent),local_table[i].next_hop,local_table[i].cost,local_table[i].time_since_update);
	}
	cout<<"Distance_Vector"<<endl;
	printf("%-1s","    ");
	printf("%-7c",(65));
	for(int i=2; i<=number_servers; i++)
	{
		printf("%-7c",(i+64));
	}
	cout<<endl;
	for(int i=1; i<=number_servers; i++)
	{
		printf("%-1c ",(i+64));
		for(int j=1; j<=number_servers; j++)
		{
			printf("%-7d",distance_vector[i][j]);
		}
		cout<<endl;
	}
	return;
}

int bellman_ford(int my_server_id, int number_servers, struct routing_table *local_table, uint16_t distance_vector[][10])				//bell-man ford calculates dv and updates local_table
{
	for(int i=1; i<=number_servers ; i++)
	{
		if(i==my_server_id)
		{
			continue;
		}
		int min=65535;
		int next_hop=-1;

		for(int j=1; j<=number_servers; j++)
		{
			if (local_table[j].isNeighbour==true && local_table[j].isEnabled==true && j!=my_server_id)
			{
				if(min > (distance_vector[my_server_id][j]+distance_vector[j][i]))
				{
					min=distance_vector[my_server_id][j]+distance_vector[j][i];
					next_hop=j;
				}
			}
		}

		distance_vector[my_server_id][i]=min;
		local_table[i].cost=min;
		local_table[i].next_hop=next_hop;

		if(distance_vector[my_server_id][i]==65535)
		{
			local_table[i].next_hop=-1;
		}
	}
	return 0;
}

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{
	/*Init. Logger*/
	cse4589_init_log();

	/* Clear LOGFILE and DUMPFILE */
	fclose(fopen(LOGFILE, "w"));
	fclose(fopen(DUMPFILE, "wb"));

	/*Start Here*/

	/*This Server Variables*/
	int number_servers, neighbours, timeout, packets_rec=0;
	int my_server_id, my_port_int;
	char *my_port, *my_ip;
	char *commandLine=(char*)malloc(1024);
	char *topologyFilePath;
	/*End of This Server Variables*/

	/*Socket Variables*/
	int rv, yes=1;
	int listener, maximumFileD;
	struct addrinfo hints, *servinfo, *p;
	/*End of Socket Variables*/

	/*Select() Variables*/
	fd_set masterSet, readSet;
	FD_ZERO(&masterSet);                   	    // clear the master and temp sets
	FD_ZERO(&readSet);
	int select_return;
	struct timeval timeout_select;
	int local_time=0;
	/*End of Select() Variables*/

	/*Check command-line Parameters*/			// http://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html#Example-of-Getopt
	if(argc!=5)
	{
		print_usage();
		exit(EXIT_FAILURE);
	}
	int option = 0;
	while ((option = getopt(argc, argv,"t:i:")) != -1)
	{
		switch (option)
		{
		case 't' :
			topologyFilePath=(char*)malloc(512);
			strcpy(topologyFilePath,optarg);										//cout<<topologyFilePath<<endl;
			break;
		case 'i' :
			timeout=strtol(optarg, NULL, 10);										//cout<<"t-out "<<timeout<<endl;
			break;
		default:
			print_usage();
			exit(EXIT_FAILURE);
		}
	}
	if(timeout <= 0 )
	{
		print_usage();
		exit(EXIT_FAILURE);
	}

	topologyFilePath[strlen(topologyFilePath)]='\0';
	ifstream topologyF(topologyFilePath);

	if(topologyF.good())
	{
		//cout<<"File Open"<<endl;
	}
	else
	{
		cerr<<"Topology file not found"<<endl;
		print_usage();
		exit(EXIT_FAILURE);
	}

	/*End of Check command-line Parameters*/

	/*Parse Topology File & Initialize Structs & Initialize Matrix for Bellman-Ford*/
	string line;
	char *buffer_local=(char*)malloc(256);
	char *buffer=(char*)malloc(256);
	if(getline(topologyF,line))
	{
		number_servers=strtol(line.c_str(),NULL,10);										//cout<<"num_servers "<<number_servers<<endl;
		line.clear();
	}
	else
	{
		cout<<"Parse Error"<<endl;
		print_usage();
		exit(EXIT_FAILURE);
	}

	if(getline(topologyF,line))
	{
		neighbours=strtol(line.c_str(),NULL,10);											//cout<<"neighbours "<<neighbours<<endl;
		line.clear();
	}
	else
	{
		cout<<"Parse Error"<<endl;
		print_usage();
		exit(EXIT_FAILURE);
	}

	struct routing_table local_table[number_servers+1];															//routing table
	uint16_t distance_vector[10][10];
	uint16_t myTopology[10];


	for(int i=1; i<=number_servers; i++)
	{																											//cout<<i<<endl;
		if(getline(topologyF,line))
		{
			int temp;
			strcpy(buffer,line.c_str());

			strcpy(buffer_local,strtok(buffer," "));															//cout<<buffer_local<<endl;
			temp=strtol(buffer_local,NULL,10);
			local_table[temp].server_id=temp;																	//cout<<local_table[i].server_id<<endl;

			strcpy(buffer_local,strtok(NULL," "));																//cout<<buffer_local<<endl;
			inet_pton(AF_INET, buffer_local, &(local_table[temp].ip_address.s_addr));							// beej

			strcpy(buffer_local,strtok(NULL," "));																//cout<<buffer_local<<endl;
			local_table[temp].port=strtol(buffer_local,NULL,10);												//cout<<local_table[i].port<<endl;

			local_table[temp].cost=inf;
			local_table[temp].isNeighbour=false;
			local_table[temp].isEnabled=true;
			local_table[temp].isPresent=false;
			local_table[temp].next_hop=-1;
			local_table[temp].time_since_update=inf;
		}
		else
		{
			cout<<"Parse Error"<<endl;
			print_usage();
			exit(EXIT_FAILURE);
		}
	}

	for(int i=1; i<=neighbours ;i++)
	{
		if(getline(topologyF,line))
		{
			strcpy(buffer,line.c_str());

			strcpy(buffer_local,strtok(buffer," "));
			my_server_id=strtol(buffer_local,NULL,10);

			strcpy(buffer_local,strtok(NULL," "));
			int neighbour_id=strtol(buffer_local,NULL,10);
			local_table[neighbour_id].isNeighbour=true;

			strcpy(buffer_local,strtok(NULL," "));
			myTopology[neighbour_id]=strtol(buffer_local,NULL,10);
		}
		else
		{
			cout<<"Parse Error"<<endl;
			print_usage();
			exit(EXIT_FAILURE);
		}
	}
	//cout<<"Myserverid "<<my_server_id<<endl;
	my_port_int=local_table[my_server_id].port;
	my_port=(char*)malloc(sizeof(int)+1);
	sprintf (my_port, "%d", my_port_int);
	local_table[my_server_id].cost=0;
	local_table[my_server_id].next_hop=my_server_id;
	local_table[my_server_id].time_since_update=0;

	my_ip=(char*)malloc(INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &(local_table[my_server_id].ip_address.s_addr), my_ip, INET_ADDRSTRLEN);
	cout<<"Router "<<my_server_id<<", listening on "<<my_ip<<":"<<my_port<<endl;

	for(int i=1; i<=number_servers; i++)
	{
		for(int j=1; j<=number_servers; j++)
		{
			if(i==j)
			{
				distance_vector[i][j]=0;
			}
			else
			{
				distance_vector[i][j]=inf;
			}
		}
	}
	/*End of Parse Topology File & Initialize Structs & Initialize Matrix for Bellman-Ford*/


	/*Create a UDP Listening Socket*/			//From Beej
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, my_port, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol))== -1)
		{
			perror("listener: socket");
			continue;
		}
		if (setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1)
		{
			perror("setsockopt");
			exit(1);
		}
		if (bind(listener, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(listener);
			perror("listener: bind");
			continue;
		}
		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "listener: failed to bind socket\n");
		return -1;
	}

	freeaddrinfo(servinfo);
	/*End of Create a UDP Listening Socket*/

	/*Select() Initialize*/
	FD_SET(listener, &masterSet);															// add the listening socket descripter to the master set
	maximumFileD = listener;																// keep track of the biggest file descriptor, so far, it's this one
	FD_SET(0, &masterSet);																	// for getting input from user
	/*End of Select() Initialize*/

	for (;;)																				// main loop
	{
		readSet = masterSet; 																// copy master set to read set
		timeout_select.tv_sec = 1;														    // set select timeout
		timeout_select.tv_usec = 0;

		select_return = select(maximumFileD + 1, &readSet, NULL, NULL, &timeout_select);	//cout<<"After Select"<<endl;

		if(select_return==-1)																//select error
		{
			cerr << "select() Failed. "<< endl;
		}
		else if (select_return==1)															//data available
		{
			if (FD_ISSET(0, &readSet))
			{																				//cout<<"user input"<<endl;
				if (fgets(commandLine, 1024, stdin) == NULL)
				{
					cerr << "fgets() Failed. "<< endl;
				}
				commandLine[strlen(commandLine) - 1] = '\0';
				if (strncasecmp(commandLine, "academic_integrity", sizeof("academic_integrity")-1) == 0)
				{
					cse4589_print_and_log("%s:SUCCESS\n", commandLine);
					char *academic_integrity;
					academic_integrity=(char*)malloc(sizeof("I have read and understood the course academic integrity policy located at http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity")+1);
					strcpy(academic_integrity, "I have read and understood the course academic integrity policy located at http://www.cse.buffalo.edu/faculty/dimitrio/courses/cse4589_f14/index.html#integrity");
					cse4589_print_and_log(academic_integrity);
					cout<<endl;
					memset(commandLine,'\0',sizeof(commandLine));
				}
				else if(strncasecmp(commandLine, "step", sizeof("step")-1) == 0)
				{
					if(send_update(listener,my_server_id, number_servers ,local_table)==0)
					{
						cse4589_print_and_log("%s:SUCCESS\n", commandLine);
					}
					else
					{
						cse4589_print_and_log("%s:%s\n", commandLine, "Update Not Sent");
					}
					memset(commandLine,'\0',sizeof(commandLine));
				}
				else if(strncasecmp(commandLine, "packets", sizeof("packets")-1) == 0)
				{
					cse4589_print_and_log("%s:SUCCESS\n", commandLine);
					cse4589_print_and_log("%d\n", packets_rec);

					packets_rec=0;
					memset(commandLine,'\0',sizeof(commandLine));
				}
				else if(strncasecmp(commandLine, "display", sizeof("display")-1) == 0)
				{
					cse4589_print_and_log("%s:SUCCESS\n", commandLine);
					printf("%-15s%-15s%-15s\n", "server_id","next_hop","cost");
					for (int i = 1; i <= number_servers; ++i)
					{
						cse4589_print_and_log("%-15d%-15d%-15d\n", local_table[i].server_id,local_table[i].next_hop,local_table[i].cost);
					}
					memset(commandLine,'\0',sizeof(commandLine));
				}
				else if(strncasecmp(commandLine, "crash", sizeof("crash")-1) == 0)
				{
					cse4589_print_and_log("%s:SUCCESS\n", commandLine);
					cerr<<"Crashed.........:("<<endl;
					for(;;)
					{

					}
					memset(commandLine,'\0',sizeof(commandLine));
				}
				else if(strncasecmp(commandLine, "dump", sizeof("dump")-1) == 0)
				{
					if(dump(my_server_id, number_servers, local_table)==0)
					{
						cse4589_print_and_log("%s:SUCCESS\n", commandLine);
					}
					else
					{
						cse4589_print_and_log("%s:%s\n", commandLine, "Error Dumping Packet Data");
					}
					memset(commandLine,'\0',sizeof(commandLine));
				}
				else if(strstr(commandLine, "disable") || strstr(commandLine, "DISABLE"))
				{
					char *temp=(char*)malloc(1024);
					strcpy(temp,commandLine);
					memset(commandLine,'\0',sizeof(commandLine));
					strcpy(commandLine,strtok(temp," "));
					int temp_id=strtol(strtok(NULL," "),NULL,10);
					if (local_table[temp_id].isNeighbour==true && local_table[temp_id].isEnabled==true)
					{
						for (int j = 1; j <=number_servers; j++)
						{
							distance_vector[temp_id][j]=inf;
						}
						distance_vector[my_server_id][temp_id]=inf;
						local_table[temp_id].isEnabled=false;
						bellman_ford(my_server_id, number_servers, local_table, distance_vector);
						cse4589_print_and_log("%s:SUCCESS\n", commandLine);
					}
					else
					{
						cse4589_print_and_log("%s:%s\n", commandLine, "Not a Valid Command");
					}
					memset(commandLine,'\0',sizeof(commandLine));
				}
				else if(strstr(commandLine, "update") || strstr(commandLine, "UPDATE"))
				{
					char *temp=(char*)malloc(1024);
					strcpy(temp,commandLine);
					memset(commandLine,'\0',sizeof(commandLine));
					strcpy(commandLine,strtok(temp," "));
					int temp_id1=strtol(strtok(NULL," "),NULL,10);
					int temp_id2=strtol(strtok(NULL," "),NULL,10);
					int temp_c;
					char *temp_cost=(char*)malloc(16);
					strcpy(temp_cost,strtok(NULL," "));
					if(temp_cost=="inf")
					{
						temp_c=inf;
					}
					else
					{
						temp_c=strtol(temp_cost,NULL,10);
					}
					if(temp_id1==my_server_id && local_table[temp_id2].isNeighbour==true)
					{
						myTopology[temp_id2]=temp_c;
						for(int i=1; i<=number_servers; i++)
						{
							local_table[i].isPresent=false;
							for(int j=1; j<=number_servers; j++)
							{
								if(i==j)
								{
									distance_vector[i][j]=0;
								}
								else
								{
									distance_vector[i][j]=inf;
								}
							}
						}
						bellman_ford(my_server_id, number_servers, local_table, distance_vector);
						cse4589_print_and_log("%s:SUCCESS\n", commandLine);
					}
					else
					{
						cse4589_print_and_log("%s:%s\n", commandLine, "Not a Valid Command");
					}
					memset(commandLine,'\0',sizeof(commandLine));
				}
				if (strncasecmp(commandLine, "state", sizeof("state")-1) == 0)										//for my debugging
				{
					state(my_server_id, number_servers, local_table, distance_vector);
					memset(commandLine,'\0',sizeof(commandLine));
				}
				else
				{
					memset(commandLine,'\0',sizeof(commandLine));
				}
			}
			else if(FD_ISSET(listener,&readSet))																							//got udp packet update
			{
				ssize_t bytes_received,size=(2*sizeof(uint16_t))+(sizeof(struct in_addr))+(number_servers*sizeof(struct entry));
				struct packet *received_packet=(packet*)malloc(size);																		//cout<<"before recvfrom"<<endl;
				struct sockaddr_in *dest;
				socklen_t destlen;
				bytes_received=recvfrom(listener, (struct packet*)received_packet, size, 0, NULL, NULL);									//cout<<" Received "<<size<<" "<<bytes_received<<endl;

				if(bytes_received==size)																									// validate packet
				{																															//cout<<"got update"<<endl;
					int remote_server_id;
					for(int i=1; i<=ntohs(received_packet->update_fields); i++)
					{
						if(ntohs(received_packet->entries[i-1].cost==0))
						{
							remote_server_id=ntohs(received_packet->entries[i-1].server_id);
						}
					}																														//cout<<" from "<<remote_server_id;
					if (local_table[remote_server_id].isEnabled==true)
					{
						packets_rec++;
						//reset timeout
						if (local_table[remote_server_id].isPresent==false || local_table[remote_server_id].time_since_update>(timeout*3))
						{
							local_table[remote_server_id].isPresent=true;
							distance_vector[my_server_id][remote_server_id]=myTopology[remote_server_id];
						}
						cse4589_print_and_log("RECEIVED A MESSAGE FROM SERVER %d\n", remote_server_id);
						for(int i=1; i<=ntohs(received_packet->update_fields); i++)
						{
							cse4589_print_and_log("%-15d%-15d\n", ntohs(received_packet->entries[i-1].server_id), ntohs(received_packet->entries[i-1].cost));
						}
						for(int i=1; i<=ntohs(received_packet->update_fields); i++)
						{
							distance_vector[remote_server_id][i]=ntohs(received_packet->entries[i-1].cost);
						}
						local_table[remote_server_id].time_since_update=0;
						bellman_ford(my_server_id, number_servers, local_table, distance_vector);
					}
				}
				memset((struct packet*)received_packet,'\0',size);
			}
		}
		else if(select_return==0)															    //select timed out
		{
			local_time++;																		//cout<<"select timeout "<<local_time<<endl;
			if(local_time==timeout)																// send update
			{
				send_update(listener,my_server_id, number_servers, local_table);
				local_time=0;
				cout<<"Sent Routing Update to Neighbors"<<endl;
			}
			for(int i=1; i<=number_servers; i++)												// check if any neighbor missed 3 consecutive updates
			{																					//cout<<"Server ID "<<local_table[i].server_id<<endl;
				if (local_table[i].isNeighbour==true && local_table[i].isEnabled==true && local_table[i].isPresent==true)
				{
					local_table[i].time_since_update++;											//cout<<"Neighbor ID "<<local_table[i].server_id<<" Time Since Last Update "<<local_table[i].time_since_update<<endl;
					if(local_table[i].time_since_update==(timeout*3))
					{
						local_table[i].next_hop=-1;
						distance_vector[my_server_id][i]=inf;
						for (int j = 1; j <=number_servers; j++)
						{
							distance_vector[i][j]=inf;
						}
						cout<<"Neighbor "<<local_table[i].server_id<<" missed 3 consecutive updates, cost set to INFINITY"<<endl;
						bellman_ford(my_server_id, number_servers, local_table, distance_vector);
					}
				}
			}
		}

	}
	return 0;
}
