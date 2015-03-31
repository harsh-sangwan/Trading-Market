#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

//if tradeitem asked for implies 0 else -1
int funcw(char *message,int sockfd,int tradeitem)
{   
    int n,num,ack;  //successful ack is -10

    printf("%s\n",message); 
    scanf("%d",&num);
    
   
    
    
    int connum = htonl(num);
    n = write(sockfd,&connum,sizeof(connum));
    if(n < 0) error("ERROR writing to socket");
    
    n = read(sockfd,&ack,sizeof(ack));
    if (n < 0) error("ERROR reading from socket");
    
    if(ntohl(ack) == -30)
        return -1;    
    
    else if(ntohl(ack) == -10)
        return 0;   //return true := return 0
    
}

//now to read the num from socket
int funcr(int sock,int var)
{
    int n;
    int num;
    int ack;    //ack = -10 if successful and -20 if unsuccessful and if out of constraint implies ack = -30
    
    n = read(sock,&num,sizeof(num));
    if (n < 0) error("ERROR reading from socket");
    
    ack = -10;
    int convack = htonl(ack);    //ack is int after converting to htonl
    n = write(sock,&convack,sizeof(convack)); 
    if (n < 0) error("ERROR writing to socket");
    
    return htonl(num);
}
    

//ADD ERROR HANDLING OF INEQUALITIES LIKE TRADE ITEM SHOULB BE [1,10]. CHECK AT SERVER AND NOT AT CLIENT.
void TradersGuide(int sockfd)
{
    int n,num,ack,i;
    
    while(1)
    {   
        if(funcw("Enter Trader Number",sockfd,-1) == -1)
        {
            printf("Wrong Kind of input. Try Again.\n");
            continue;
        }
            
        else
        {
            printf("Looged in successfully\n");
            break;
        }
    }
  
    
    int choice;
    
    while(1)
    {
        printf("1. Send Buy Request\n");
        printf("2. Send Sell Request\n");
        printf("3. View Order Status\n");
        printf("4. View Trade Status\n");
        printf("5. Exit\n");
        
        if(funcw("Choose the option Now",sockfd,-1) == 0)
            ;
        
        int choice = funcr(sockfd,0);
        
        switch(choice)
        {
        	case 1:
               
               	printf("\nBuy Request\n");
               	if(funcw("Enter the Trade Item",sockfd,0) == 0)
               	{
					if(funcw("Enter the Quantity",sockfd,0) == 0)
					{
						if(funcw("Enter the Price",sockfd,0) == 0)
						{

						}
						else
				            printf("Sorry, Wrong kind of input for Price\n");
				
					}
					else
				        printf("Sorry, Wrong kind of input for Quantity\n");
				}
				else
				    printf("Sorry, Wrong kind of input for Trade Item\n");
               
               break;
            
            case 2:

               	printf("\nSell Request\n");
               	if(funcw("Enter the Trade Item",sockfd,0) == 0)
               	{
					if(funcw("Enter the Quantity",sockfd,0) == 0)
					{
						if(funcw("Enter the Price",sockfd,0) == 0)
						{

						}
						else
				            printf("Sorry, Wrong kind of input for Price\n");
				
					}
					else
				        printf("Sorry, Wrong kind of input for Quantity\n");
				}
				else
				    printf("Sorry, Wrong kind of input for Trade Item\n");
				
               	break;
            
            case 3:
                printf("\nMarket Position\n");
                printf("TradeItem\tSell Price\tSell Quantity\tBuy price\tBuy Quantity\n");
                
                for(i = 0 ; i < 10 ; i++)
                {
                    printf("%d\t\t",i+1);
                    
					int sellprice = funcr(sockfd,0);
                    printf("%d\t\t",sellprice);
					int sellquant = funcr(sockfd,0);
					printf("%d\t\t",sellquant);
                    
					int buyprice = funcr(sockfd,0);
                    printf("%d\t\t",buyprice);
					int buyquant = funcr(sockfd,0);
					printf("%d\n",buyquant);

                }
                
                break;
            
            case 4:
                printf("\nYour Trade\n");
                printf("Bought / Sold\tCost\tQuantity\tSecondTrader\n");
                
                for(i = 0 ; i < 20 ; i++)
                {
                    int entry = funcr(sockfd,0);
                    if(entry == -10)    //implies no more entry
                        break;
                                       
                    else
                    {
                        if(entry == 0)  //implies buy request
                            printf("Bought\t");
                        else
                            printf("Sold\t");
                        
                        entry = funcr(sockfd,0);    //Cost
                            fprintf(stdout,"%d\t",entry);
                        
                        entry = funcr(sockfd,0);    //Quantity
                            fprintf(stdout,"%d\t",entry);
                            
                        entry = funcr(sockfd,0);    //SecondTrader
                            fprintf(stdout,"%d\t",entry);
                    }
                }
               break;
               
            case 5:

               
               exit(0);
               break;
                    
            default:
                printf("Sorry Invalid choice. Try Again");
                continue;
         }
                    
    }    
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    
    if(argc < 3) 
    {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
        
    server = gethostbyname(argv[1]);
    if (server == NULL) 
    {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    
    
    TradersGuide(sockfd);
    
    close(sockfd);
    
    return 0;
}



