/* A simple server in the internet domain using TCP. The port number is passed as an argument.
   This version runs forever, forking off a separate process for each connection */
   
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

//keeps the properties of every item no (default items set to be max)
struct Trader
{
    int TraderNo;	//Which traderno made a request for the item 
    int Quantity;	//At which quantity
    int Price;    	//At what price
};

//keeps the properties of trade of every Trader
struct Trader2
{
    int SecondTrader;	//Trader No. from whom buying/selling was done.
    int Quant;		//How much quantity traded
    int Cost;		//At what Cost Traded
    int BuySell;    	//Buy implies 0 else 1 if sell
};

void dostuff(int sock);	//Actually does all the work

//This function makes temporary files to write into. File I/O enables easier flow of synchronization among the items traded across traders
void filew(struct Trader buy[10][20], struct Trader sell[10][20], struct Trader2 status[5][20]);

//simple error message
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno, pid;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;

     if (argc < 2) 
     {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     
    //10 items each has properties as in struct; one for sell and for buy queue. Only 20 requests allowed per item no. (for simplicity)
    struct Trader buy[10][20];
    struct Trader sell[10][20];
    
    struct Trader2 status[5][20];
    int i,j;  
    
    //initialization to keep which entries aren't yet filled
    for(i = 0 ; i < 10 ; i ++)
    {
        for(j = 0 ; j < 20 ; j++)
        {
            buy[i][j].TraderNo = sell[i][j].TraderNo = 0;
            buy[i][j].Quantity = sell[i][j].Quantity = 0;
            buy[i][j].Price = 0;
            sell[i][j].Price = 9999999;
            if(i < 5)
                status[i][j].SecondTrader = 0;
        }
    }
   
    filew(buy,sell,status);	//write the data into temporary files

 

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
        
     
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     
     //binds the server struct to sock descriptor
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
             
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     
    
     while (1) 
     {
         newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		 fprintf(stdout,"New client\n");
     
         if (newsockfd < 0) 
             error("ERROR on accept");
             
         pid = fork();
         if (pid < 0)
             error("ERROR on fork");
             
         if (pid == 0)  
         {
             close(sockfd);
             dostuff(newsockfd);//,buy,sell,status);
             exit(0);
         }
         else close(newsockfd);
     } 
     
     close(sockfd);
     
     return 0; /* we never get here */
}

//to check if traderno, quantity etc. all are in their constraints change the second arg
//traderno : 0 , tradeitem : 1 , quantity : 2 , price : 3 , choice == 4
//function for repititive read and sending ack. returns the int which it read from the socket.
int funcr(int sock,int var)
{
    int n;
    int num;
    int ack;    //ack = -10 if successful and -20 if unsuccessful and if out of constraint implies ack = -30
    
    n = read(sock,&num,sizeof(num));
    if (n < 0) error("ERROR reading from socket");
    
    //doesn't work for price and quantity as of now (!!!)
    ack = -10;
    if(ntohl(num) < 1)
    {
        num = -1;
        ack = -30;
    }

    //check if Trader login is out of constraint ( ! > 5 users)	
    if(var == 0)
    {
        if(ntohl(num) > 5)  //if out of constraints implies num = -1
        {
            num = -1;
            ack = -30;
        }
    }
    
    //check if Trading Items are out of constraint ( ! > 10 items)	
    else if(var == 1)
    {
        if(ntohl(num) > 10)  //if out of constraints implies num = -1
        {
            num = -1;
            ack = -30;
        }
    }
    
    int convack = htonl(ack);    //ack is int after converting to htonl
    n = write(sock,&convack,sizeof(convack)); 	//write to socket
    if (n < 0) error("ERROR writing to socket");
    
    return htonl(num);
}

//writes a num to the socket and gets the ack from client upon successfully recieving the msg
int funcw(int sockfd,int num)
{
    int n,ack;
    
    int connum = htonl(num);
    n = write(sockfd,&connum,sizeof(connum));
    if(n < 0) error("ERROR writing to socket");
    
    n = read(sockfd,&ack,sizeof(ack));
    if (n < 0) error("ERROR reading from socket");
    
    if(ntohl(ack) == -10)   //-10 implies successfully recieved by the client
        return 0;   //return true := return 0
}

//returns the minimum selling price of the input tradeitem amongst all request made for that item
int MinSellPrice(struct Trader sell[10][20], int tradeitem)
{
    int i;
    
    int minprice = 9999999;
    int minindex;
    
    for(i = 0 ; i < 20 ; i++)
    {
        if(sell[tradeitem][i].Price < minprice)
        {
            minprice = sell[tradeitem][i].Price;
            minindex = i;
        }
    }
    
    //no entries till yet
    if(minprice == 9999999)
        return -1;
    else
        return minindex;
}

//returns the maximum buyin price of the input tradeitem amongst all request made for that item.
int MaxBuyPrice(struct Trader buy[10][20], int tradeitem)
{
    int i;
    
    int maxprice = 0;
    int maxindex;
    
    for(i = 0 ; i < 20 ; i++)
    {
        if(buy[tradeitem][i].Price > maxprice)
        {
            maxprice = buy[tradeitem][i].Price;
            maxindex = i;
        }
    }
    
    //no entries till yet
    if(maxprice == 0)
        return -1;
    else
        return maxindex;
}

//prints data for server to look
void printer(struct Trader buy[10][20], struct Trader sell[10][20], struct Trader2 status[5][20])
{
	int i,j;	

	printf("Buy Queue\t\t\tSell Queue\n");
	for(i = 0 ; i < 10 ; i++)
	{
			for(j = 0 ; j < 20 ; j++)
			{
					if(buy[i][j].TraderNo != 0 || sell[i][j].TraderNo != 0)
					printf("Item : %d ; T : %d ; Q : %d ; P : %d\t\t T : %d ; Q : %d ; P : %d\n",i,buy[i][j].TraderNo, buy[i][j].Quantity, buy[i][j].Price,sell[i][j].TraderNo,sell[i][j].Quantity,sell[i][j].Price);

			}
	}

	printf("\nTrader Status\n");
	printf("Entity\tValue\n");
	for(i = 0 ; i < 5 ; i++)
	{
			for(j = 0 ; j < 20 ; j++)
			{
					if(status[i][j].SecondTrader != 0)
					printf("trader : %d ; T : %d ; Q : %d ; P : %d ; BS : %d\n",i+1,status[i][j].SecondTrader,status[i][j].Quant,status[i][j].Cost,status[i][j].BuySell);
			}
	}

}

//writes to the files
void filew(struct Trader buy[10][20], struct Trader sell[10][20], struct Trader2 status[5][20])
{
	FILE *fp1 = fopen("trade1.txt","w");
	FILE *fp2 = fopen("trade2.txt","w");
	
	int i, j;

	for(i = 0 ; i < 10 ; i ++)
	{
			for(j = 0 ; j < 20 ; j++)
			{
				//Item ; Buy : {TraderNo ; Quantity ; Price} ; Sell : {TraderNo ; Quantity ; Price}
				fprintf(fp1,"%d %d %d %d %d %d %d\n", i, buy[i][j].TraderNo, buy[i][j].Quantity, buy[i][j].Price, sell[i][j].TraderNo,sell[i][j].Quantity,sell[i][j].Price);
			}
	}

	for(i = 0 ; i < 5 ; i++)
	{
			for(j = 0 ; j < 20 ; j++)
			{
				//Trader : 2nd Trader : Quant : Price : Buy Sell
				fprintf(fp2,"%d %d %d %d %d\n", i, status[i][j].SecondTrader, status[i][j].Quant, status[i][j].Cost, status[i][j].BuySell);
			}
	}

	fclose(fp1);
	fclose(fp2);
}

//There is a separate instance of this function for each connection. It handles all communication once a connnection has been established.
void dostuff (int sock/*, struct Trader buy[10][20], struct Trader sell[10][20], struct Trader2 status[5][20]*/)
{
    int i, j, index;
    
    int traderno;
    
    while(1)
    {
        traderno = funcr(sock,0);
        if(traderno == -1)
        {
            fprintf(stdout,"Wrong kind of TraderNo Given as Input\n");
            continue;
        }
        else
        {
            fprintf(stdout,"Trader No : %d Logged in successfully\n",traderno);
            break;
        }
    }
    
    //10 items each has properties as in struct; one for sell and for buy queue. Only 20 requests allowed per item no. (for simplicity)
    struct Trader buy[10][20];
    struct Trader sell[10][20];
    
    struct Trader2 status[5][20];

    int tradeitem, quantity, price;
    
    while(1)
    {
        /**/   
        int choice = funcr(sock,4);  
        if(funcw(sock,choice) == 0)  
            fprintf(stdout,"Trader No : %d chose the option : %d\n",traderno,choice);
        
		//file read fcn
	FILE *fp1 = fopen("trade1.txt","r");
	FILE *fp2 = fopen("trade2.txt","r");
	
	for(i = 0 ; i < 10 ; i ++)
	{
		for(j = 0 ; j < 20 ; j++)
		{
			int item,trnb,qb,pb,trns,qs,ps;
			//Item ; Buy : {TraderNo ; Quantity ; Price} ; Sell : {TraderNo ; Quantity ; Price}
			fscanf(fp1,"%d %d %d %d %d %d %d", &item, &trnb, &qb, &pb, &trns, &qs, &ps);
			buy[i][j].TraderNo = trnb;
			buy[i][j].Quantity = qb;
			buy[i][j].Price = pb;

			sell[i][j].TraderNo = trns;
			sell[i][j].Quantity = qs;
			sell[i][j].Price = ps;
		}
	}

	for(i = 0 ; i < 5 ; i++)
	{
		for(j = 0 ; j < 20 ; j++)
		{
			int item,sec,q,c,bs;
			//Trader : 2nd Trader : Quant : Price : Buy Sell
			fscanf(fp2,"%d %d %d %d %d", &item, &sec, &q, &c, &bs);
			status[i][j].SecondTrader = sec;
			status[i][j].Quant = q;
			status[i][j].Cost = c;
			status[i][j].BuySell = bs;
		}
	}

	fclose(fp1);
	fclose(fp2);

        switch(choice)
        {
        	case 1:                
                
                tradeitem = funcr(sock,1);
                if(tradeitem == -1)
                {
                    printf("Sorry, Wrong kind of input for Trade Item\n");       
                    break;
                }
                fprintf(stdout,"Trader No : %d wants to buy Item : %d\n",traderno,tradeitem);
                    
                quantity = funcr(sock,2);
                if(quantity < 0)
                {
                    printf("Sorry, Wrong kind of input for Quantity\n");       
                    break;
                }
                fprintf(stdout,"Trader No : %d wants to buy : %d Quantity of Item : %d\n",traderno,quantity,tradeitem);
                    
                price = funcr(sock,3);
                if(price < 0)
                {
                    printf("Sorry, Wrong kind of input for Price\n");       
                    break;
                }
                fprintf(stdout,"Trader No : %d wants to buy Item : %d at Price : %d for Quantity : %d\n",traderno,tradeitem,price,quantity);
                
                
                while(1)
                {
                    index = MinSellPrice(sell, tradeitem-1); //returns the request no for min. price for the item no.  
					
	            printer(buy,sell,status);

                    if(index != -1 && sell[tradeitem-1][index].Price < price)   //if price in sell queue is less than request
                    {
                        if(sell[tradeitem-1][index].Quantity == quantity)   //quantity matches
                        {
                                            
                            for(i = 0 ; i < 20 ; i++)
                            {
                                if(status[traderno-1][i].SecondTrader == 0)
                                    break;
                            }
                            
                            //first trader bought                    
                            status[traderno-1][i].SecondTrader = sell[tradeitem-1][index].TraderNo;
                            status[traderno-1][i].Quant        = sell[tradeitem-1][index].Quantity;
                            status[traderno-1][i].Cost         = sell[tradeitem-1][index].Price;
                            status[traderno-1][i].BuySell      = 0; //bought
                            
                            //Now add the trade to second trader as sold
                            int sectrader = sell[tradeitem-1][index].TraderNo;
                            status[sectrader-1][i].SecondTrader = traderno;
                            status[sectrader-1][i].Quant        = sell[tradeitem-1][index].Quantity;
                            status[sectrader-1][i].Cost         = sell[tradeitem-1][index].Price;
                            status[sectrader-1][i].BuySell      = 1; //sold
                            
                            //The request has been traded thus all the entries can be set to zero
                            sell[tradeitem-1][index].TraderNo = 0;
                            sell[tradeitem-1][index].Quantity = 0;
                            sell[tradeitem-1][index].Price    = 9999999;
                            filew(buy,sell,status);

			    printer(buy,sell,status);

                            break;
                        }
                        
                        else if(sell[tradeitem-1][index].Quantity > quantity)   //quantity of sell is greater
                        {
                            sell[tradeitem-1][index].Quantity -= quantity;  //sold quantity has to be subtracted
                            
                            //first trader bought                    
                            status[traderno-1][i].SecondTrader = sell[tradeitem-1][index].TraderNo;
                            status[traderno-1][i].Quant        = quantity;
                            status[traderno-1][i].Cost         = sell[tradeitem-1][index].Price;
                            status[traderno-1][i].BuySell      = 0; //bought
                            
                            //Now add the trade to second trader as sold
                            int sectrader = sell[tradeitem-1][index].TraderNo;
                            status[sectrader-1][i].SecondTrader = traderno;
                            status[sectrader-1][i].Quant        = quantity;
                            status[sectrader-1][i].Cost         = sell[tradeitem-1][index].Price;
                            status[sectrader-1][i].BuySell      = 1; //sold
                            filew(buy,sell,status);
			    printer(buy,sell,status);

                            break;
                
                        }
                        
                        else if(sell[tradeitem-1][index].Quantity < quantity)   //quantity of sell is lesser
                        {
                            quantity -= sell[tradeitem-1][index].Quantity;
                                                        
                            //The request has been traded thus all the entries can be set to zero
                            sell[tradeitem-1][index].TraderNo = 0;
                            sell[tradeitem-1][index].Quantity = 0;
                            sell[tradeitem-1][index].Price    = 9999999;
                            filew(buy,sell,status);

			    printer(buy,sell,status);
							
                            continue;   //check again for other prices.
                        
                        }
                    }     
                    
                    //no entries in sell till now or price is greater
                    else    //append request to buy queue
                    {
                        for(i = 0 ; i < 20 ; i++)
                        {
                            if(buy[tradeitem-1][i].TraderNo == 0)
                                break;
                        }
                        
                        buy[tradeitem-1][i].TraderNo = traderno;
                        buy[tradeitem-1][i].Quantity = quantity;
                        buy[tradeitem-1][i].Price    = price;
                       	filew(buy,sell,status);

			printer(buy,sell,status);
                        break;
                    }
                }                    
                break;

            case 2:
                tradeitem = funcr(sock,1);
                if(tradeitem == -1)
                {
                    printf("Sorry, Wrong kind of input for Trade Item\n");       
                    break;
                }
                fprintf(stdout,"Trader No : %d wants to sell Item : %d\n",traderno,tradeitem);
                    
                quantity = funcr(sock,2);
                if(quantity < 0)
                {
                    printf("Sorry, Wrong kind of input for Quantity\n");       
                    break;
                }
                fprintf(stdout,"Trader No : %d wants to sell : %d Quantity of Item : %d\n",traderno,quantity,tradeitem);
                    
                price = funcr(sock,3);
                if(price < 0)
                {
                    printf("Sorry, Wrong kind of input for Price\n");       
                    break;
                }
                fprintf(stdout,"Trader No : %d wants to sell Item : %d at Price : %d for Quantity : %d\n",traderno,tradeitem,price,quantity);
                
                
                while(1)
                {
                    index = MaxBuyPrice(buy, tradeitem-1); //returns the request no for max. buy price for the item no.  
                    printer(buy,sell,status);

                    if(index != -1 && buy[tradeitem-1][index].Price > price)   //if price in buy queue is more than request
                    {
                        if(buy[tradeitem-1][index].Quantity == quantity)   //quantity matches
                        {
                                            
                            for(i = 0 ; i < 20 ; i++)
                            {
                                if(status[traderno-1][i].SecondTrader == 0)
                                    break;
                            }
                            
                            //first trader sold                    
                            status[traderno-1][i].SecondTrader = buy[tradeitem-1][index].TraderNo;
                            status[traderno-1][i].Quant        = buy[tradeitem-1][index].Quantity;
                            status[traderno-1][i].Cost         = buy[tradeitem-1][index].Price;
                            status[traderno-1][i].BuySell      = 1; //sold
                            
                            //Now add the trade to second trader as bought
                            int sectrader = buy[tradeitem-1][index].TraderNo;
                            status[sectrader-1][i].SecondTrader = traderno;
                            status[sectrader-1][i].Quant        = buy[tradeitem-1][index].Quantity;
                            status[sectrader-1][i].Cost         = buy[tradeitem-1][index].Price;
                            status[sectrader-1][i].BuySell      = 0; //bought
                            
                            //The request has been traded thus all the entries can be set to zero
                            buy[tradeitem-1][index].TraderNo = 0;
                            buy[tradeitem-1][index].Quantity = 0;
                            buy[tradeitem-1][index].Price    = 0;
                            printf("traded\n");
                    	    filew(buy,sell,status);

	     		    printer(buy,sell,status);
                            break;
                        }
                        
                        else if(buy[tradeitem-1][index].Quantity > quantity)   //quantity of sell is greater
                        {
                            buy[tradeitem-1][index].Quantity -= quantity;  //sold quantity has to be subtracted
                            
                            //first trader sold                    
                            status[traderno-1][i].SecondTrader = buy[tradeitem-1][index].TraderNo;
                            status[traderno-1][i].Quant        = quantity;
                            status[traderno-1][i].Cost         = buy[tradeitem-1][index].Price;
                            status[traderno-1][i].BuySell      = 1; //sold
                            
                            //Now add the trade to second trader as bought
                            int sectrader = buy[tradeitem-1][index].TraderNo;
                            status[sectrader-1][i].SecondTrader = traderno;
                            status[sectrader-1][i].Quant        = quantity;
                            status[sectrader-1][i].Cost         = buy[tradeitem-1][index].Price;
                            status[sectrader-1][i].BuySell      = 0; //bought
                            printf("quant 2\n");
                            filew(buy,sell,status);

			    printer(buy,sell,status);
			    break;
                            
                        }
                        
                        else if(buy[tradeitem-1][index].Quantity < quantity)   //quantity of sell is lesser
                        {
                            quantity -= buy[tradeitem-1][index].Quantity;
                                                        
                            //The request has been traded thus all the entries can be set to zero
                            buy[tradeitem-1][index].TraderNo = 0;
                            buy[tradeitem-1][index].Quantity = 0;
                            buy[tradeitem-1][index].Price    = 0;
                            printf("quant 3\n");
                            filew(buy,sell,status);

			    printer(buy,sell,status);
			    continue;   //check again for other prices.
                        
                        }
                    }     
                    
                    //no entries in sell till now or price is greater
                    else    //append request to buy queue
                    {
                        for(i = 0 ; i < 20 ; i++)
                        {
                            if(sell[tradeitem-1][i].TraderNo == 0)
                                break;
                        }
                        
                        sell[tradeitem-1][i].TraderNo = traderno;
                        sell[tradeitem-1][i].Quantity = quantity;
                        sell[tradeitem-1][i].Price    = price;
                        printf("appended\n");
                       	filew(buy,sell,status);

			printer(buy,sell,status);
			break;
                    }
                }                    
                break;

                
            case 3:
                for(i = 0 ; i < 10 ; i++)
                {
                    index = MinSellPrice(sell, i); //returns the request no for min. sell price for the item no.
                    if(funcw(sock, sell[i][index].Price) == 0) ;
			if(funcw(sock, sell[i][index].Quantity) == 0) ;

                    index = MaxBuyPrice(buy, i); //returns the request no for max. buy price for the item no.
                    if(funcw(sock, buy[i][index].Price) == 0) ;
			if(funcw(sock, buy[i][index].Quantity) == 0) ;
                    
                }
                
                break;
                
            case 4:
                for(i = 0 ; i < 20 ; i++)
                {
                    if(status[traderno-1][i].SecondTrader == 0) //No entries
                    {
                        if(funcw(sock, -10) == 0) ;       //-10 implies no entry
                        	break;
                    }
                    
                    else
                    {
                        if(funcw(sock, status[traderno-1][i].BuySell) == 0) ;       //write if buy/sell
                        if(funcw(sock, status[traderno-1][i].Cost) == 0) ;          //write Price of the trade
                        if(funcw(sock, status[traderno-1][i].Quant) == 0) ;         //write the quantity traded
                        if(funcw(sock, status[traderno-1][i].SecondTrader) == 0) ;  //write the Second Trader involved
                    }
                }
                
                break; 
           
           case 5:
                printf("Trader No %d has exited\n",traderno);
                break; 
           
           default:
                printf("Wrong kind of input chosen by the trader\n");
                break;               
        }
        
    }

}
