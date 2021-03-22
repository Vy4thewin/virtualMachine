#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Global variables list
int pc=0,sp=0, prevReg,reg1,reg2, opcode[2], result[2], op1[2], op2[2], currinstruction[4],currinstruction2[4],*registers[16],finalreg[2],offset[2],regtoLoS[2],addregister[2],PuOrP0[2], pupOreg[2], immed[2], memOrStore, branch[2];
unsigned char * memory[1024];
bool flag= false, prinMemorReg=false, fetchI1=true, decodeI1=false, executeI1=false, storeI1= false, fetchI2=false, decodeI2=false, executeI2=false,storeI2=false;
/* currinstructions acts like our data bus, loads the current instructions bytes on a int arrays to use thr the cpu,so memory data isnt affected by changes 
all registers used are expanded by 2 to fit two current instructions,seperate by [0] and [1]
for CI1 its [0] and CI2 [1], locked and only can be fill/used those position if their buffers are true
*/

void load(const char * f){
	FILE *file = fopen(f,"r");
	if(file==0){
		printf("unable to open file");
		exit(1);
	}
	fread(&memory,1,1024,file);
	// file the memory with the input untill it reaches the end of file or fills 1024 bytes
	fclose(file);
}

void fetch(){
	if(fetchI1==true){	
		currinstruction[0]=atoi((const char *)memory[pc]);
		opcode[0]= (currinstruction[0])>>4;
		// gets the integer value of byte[0] and finds the CI1's opcode

		if(opcode[0]!=0){
				if(opcode[0] <= 6|| opcode[0] >= 8 ){
					currinstruction[1]=atoi((const char *)memory[pc++]);	
			}else{
				for(int a=0; a<3;a++){
					currinstruction[a+1]= atoi((const char *)memory[pc++]);
				}
			}
			// If CI1's opcode is one of the 2 bytes instructions, it increments pc to fill with the next byte in memory thats byte[1]
			//else, the for loop will increment pc and fill the rest of the 3 bytes from memory 
		}else{
			flag= true;
		}
		// if the opcode is not zero, follow with the fetch
		// if opcode is zero, don't fetch and set the flag to true
		decodeI1=true;
		// send signal to decode a OK to decode for current instruction 1
	}
	if(fetchI2==true){	
		currinstruction2[0]=atoi((const char *)memory[pc]);
		opcode[1]= (currinstruction2[0])>>4;
		// gets the integer value of byte[0] and finds the opcode

		if(opcode[1]!=0){
				if(opcode[1] <= 6|| opcode[1] >= 8 ){
					currinstruction2[1]=atoi((const char *)memory[pc++]);	
			}else{
				for(int a=0; a<3;a++){
					currinstruction2[a+1]= atoi((const char *)memory[pc++]);
				}
			}
			// If the opcode is one of the 2 bytes instructions, it increments pc to fill with the next byte in memory thats byte[1]
			//else, the for loop will increment pc and fill the rest of the 3 bytes from memory 
		}else{
			flag= true;
		}
		// if the opcode is not zero, follow with the fetch
		// if opcode is zero, don't fetch and set the flag to true
		
		decodeI2=true;
		// send signal to decode OK to decode for current instruction 2
	}
	/*if statement only allow current instruction that are allowed to work through booleans 
	  If the instructions are signal true, then it's allow to process its work while the another instrcutions waits for its signal
	  When the allowed instruction is finished with its work, it sends a signals ok for the nest stage work,set up for all stages of the cpu
	  first round, CI1 goes first and CI2 follow unless there's a branch, then other rules apply
	  */
}


void decode(){
	if(decodeI1==true){
		fetchI1=false;
		fetchI2=true;
		// decode sends signal to not decode for current instruction 1 if CI1 is currently decoding
		//but sends signal to Current Instruction 2 it's okay to fetch while CI1 is decoding
		
	if(opcode[0]<=6){
		reg1=(currinstruction[0])& 0xF;
		reg2=((currinstruction[1])& 0xF0)>>4;
		op1[0]= *registers[reg1];
		op2[0]= *registers[reg2];
		finalreg[0]=(currinstruction[1])&0xF;
		//For 3R instructions,  get registers ready for whatever work it does for execute
		//finalreg is the index of register the value of CI1's op1 and op2 will go to, used in store ( also bracnh forwarding if needed with reg1/reg2)
		
	}else if (opcode[0]==7){
		 branch[0]= (currinstruction[0])&0xF;
		//find branch to seperate actions for br,jump, and call
			if(branch[0]<=5){
				reg1=((currinstruction[1])& 0xF0)>>4;
				reg2=(currinstruction[1])& 0xF;
				op1[0]= *registers[reg1];
				op2[0]= *registers[reg2];
				offset[0]=((currinstruction[2])<<8)| currinstruction[3];
				//To find the offset again, we have to bind the two bytes back together for the orginial offset. So pushing byte[2] left 8 and OR it to byte[3]
			}else if(branch[0]==6){
				offset[0]=(((currinstruction[2])<<8)| currinstruction[3])*2;
				memory[0]=(unsigned char *)&pc;
				// records th pc of the current instruction for call, as it pushes into the "botttom" of the stack
				//will be used for return instruction
				
			}else{
				offset[0]=(((currinstruction[2])<<8)| currinstruction[3])*2;
			// gets the offset for jump like br1 and call	
			}
	}else if(opcode[0]== 8||opcode[0]==9){
		regtoLoS[0]=(currinstruction[0])&0xF;
		addregister[0]=((currinstruction[1])& 0xF0)>>4;
		//regtoLoS used for load/store, addregister is the reg that value is being used. Used in execute 
		
		offset[0]=(currinstruction[1])& 0xF;
		// the offset for load/store, used in the store stage
	}else if(opcode[0]==10){
		pupOreg[0]=((currinstruction[0])& 0xF0)>>4;
		PuOrP0[0]=((currinstruction[1])& 0xF0)>>6;
		//PushorPop gets the value of which operation it's doing

	}else if (opcode[0]==11){
		finalreg[0]=(currinstruction[0])& 0xF;
		immed[0]=currinstruction[1];
		// Gets register to fill in immediate value into for store
	}else{
		memOrStore=currinstruction[1];
		//No registers involved, just need immediate value to determine what to print 	
	}
	executeI1=true;
	// send signal OK to execute for current instruction 1
	
	}
	if(decodeI2==true){
		fetchI1=true;
		fetchI2=false;
		//signals CI1 is okay to fetch while I2 is decoding
		//signals CI2 is not okay to fetch while I2 is decoding 
	if(opcode[1]<=6){
		
		reg1=(currinstruction2[0])& 0xF;
		reg2=((currinstruction2[1])& 0xF0)>>4;
		op1[1]= *registers[reg1];
		op2[1]= *registers[reg2];
		finalreg[1]=(currinstruction2[1])&0xF;
		//For 3R instructions,  get registers ready for whatever work it does for execute
		//finalreg is the index of register the value of CI2's op1 and op2 will go to, used in store
		
	}else if (opcode[1]==7){
		 branch[1]= (currinstruction2[0])&0xF;
		//find branch to seperate actions for br,jump, and call
			if(branch[1]<=5){
				reg1=((currinstruction2[1])& 0xF0)>>4;
				reg2=(currinstruction2[1])& 0xF;
				op1[1]= *registers[reg1];
				op2[1]= *registers[reg2];
				offset[1]=((currinstruction2[2])<<8)| currinstruction2[3];
				//To find the offset again, we have to bind the two bytes back together for the orginial offset. So pushing byte[2] left 8 and OR it to byte[3]
				
			}else if(branch[1]==6){
				offset[1]=(((currinstruction2[2])<<8)| currinstruction2[3])*2;
				memory[0]=(unsigned char *)&pc;
				// records th pc of the current instruction for call, as it pushes into the "botttom" of the stack
				//will be used for return instruction
				
			}else{
				offset[1]=(((currinstruction2[2])<<8)| currinstruction2[3])*2;
			// gets the offset for jump like br1 and call	
			}
	}else if(opcode[1]== 8||opcode[1]==9){
		regtoLoS[1]=(currinstruction2[0])&0xF;
		addregister[1]=((currinstruction2[1])& 0xF0)>>4;
		//regtoLoS used for load/store, addregister is the reg that value is being used. Used in execute 
		
		offset[1]=(currinstruction2[1])& 0xF;
		// the offset for load/store, used in the store stage
	}else if(opcode[1]==10){
		pupOreg[1]=((currinstruction2[0])& 0xF0)>>4;
		PuOrP0[1]=((currinstruction2[1])& 0xF0)>>6;
		//PushorPop gets the value of which operation it's doing

	}else if (opcode[1]==11){
		finalreg[1]=(currinstruction2[0])& 0xF;
		immed[1]=currinstruction2[1];
		// Gets register to fill in immediate value into for store
	}else{
		memOrStore=currinstruction2[1];
		//No registers involved, just need immediate value to determine what to print 	
	}
		executeI2=true;
	// send signal OK to execute for current instruction 2
	}
}


void execute(){
	if(executeI1==true){
	decodeI1=false;
	decodeI2=true;
	//signal OK to decode C2 while I1 is executing
	//signal I1 not okay to decode while I1 is executing
	
	if(prevReg==reg1){
		op1[0]=*registers[prevReg];
	}
	if(prevReg==reg2){
		op2[0]=*registers[prevReg];
	}
	//if op1 and op2 for current instruction 1 was the same register that the outputted register in CI2 in store, replace the value of ops with the value of the final register
	
	switch(opcode[0]){
		case 1:
			result[0]=op1[0]+op2[0];
			break;
		case 2:
			result[0]= op1[0]&op2[0];
			break;
		case 3:
			result[0]= op1[0]/op2[0];
			break;
		case 4:
			result[0]=op1[0]*op2[0];
			break;
		case 5:
			result[0]=op1[0]-op2[0];
			break;
		case 6:
			result[0]=op1[0]|op2[0];
			break;
		case 7:
			switch(branch[0]){
			case 0:
				if((op1[0]<op2[0])==1){
					result[0]=1;
				}else{
					result[0]=0;
				}
				break;
			case 1:
				if((op1[0]<=op2[0])==1){
					result[0]=1;
				}else{
					result[0]=0;
				}
				break;
			case 2:
				if((op1[0]==op2[0])==1){
					result[0]=1;
				}else{
					result[0]=0;
				}
				break;
			case 3:
				if((op1[0]!=op2[0])==1){
					result[0]=1;
				}else{
					result[0]=0;
				}
				break;
			case 4:
				if((op1[0]>op2[0])==1){
					result[0]=1;
				}else{
					result[0]=0;
				}
				break;
			default:
				if((op1[0]>=op2[0])==1){
					result[0]=1;
				}else{
					result[0]=0;
				}
				break;
			}
			break;
			// for all Branches, there's two switches. First switch turns on for op7, the second switch is based on the branch type
			// if the branch is correct, it will send a true thr CI1's result for store, else it will send a false thr. result
		case 12:
			prinMemorReg=true;
			// throws flag for printing whatever data after loop
			break;
		default:
			break;
	}
	// cases 1-6 are one switch waiting for the current instruction opcode to turn of whatver calculation it needs to do to be sent to store 	
	//12 swtiches on the flag to be set for true for printing data after cpu is done
	
	storeI1=true;
	//send signal OK to store CI1
	
	}
	if(executeI2==true){
		decodeI2=false;
		decodeI1=true;
		//signal Ok for C1 to decode while CI2 is executing
		//signal Not okay for CI2 to decode while C2 is storing
		
		if(prevReg==reg1){
		op1[1]=*registers[prevReg];
		}
		if(prevReg==reg2){
		op2[1]=*registers[prevReg];
		}
		//if op1 and op2 for current instruction 1 was the same register that the outputted register in CI2 in store, replace the value of ops with the value of the final register
		
		switch(opcode[1]){
		case 1:
			result[1]=op1[1]+op2[1];
			break;
		case 2:
			result[1]= op1[1]&op2[1];
			break;
		case 3:
			result[1]= op1[1]/op2[1];
			break;
		case 4:
			result[1]=op1[1]*op2[1];
			break;
		case 5:
			result[1]=op1[1]-op2[1];
			break;
		case 6:
			result[1]=op1[1]|op2[1];
			break;
		case 7:
			switch(branch[1]){
			case 0:
				if((op1[1]<op2[1])==1){
					result[1]=1;
				}else{
					result[1]=0;
				}
				break;
			case 1:
				if((op1[1]<=op2[1])==1){
					result[1]=1;
				}else{
					result[1]=0;
				}
				break;
			case 2:
				if((op1[1]==op2[1])==1){
					result[1]=1;
				}else{
					result[1]=0;
				}
				break;
			case 3:
				if((op1[1]!=op2[1])==1){
					result[1]=1;
				}else{
					result[1]=0;
				}
				break;
			case 4:
				if((op1[1]>op2[1])==1){
					result[1]=1;
				}else{
					result[1]=0;
				}
				break;
			default:
				if((op1[1]>=op2[1])==1){
					result[1]=1;
				}else{
					result[1]=0;
				}
				break;
			}
			break;
			// for all Branches, there's two switches. First switch turns on for op7, the second switch is based on the branch type
			// if the branch is correct, it will send a true thr  CI2's result for store, else it will send a false thr. result
		case 12:
			prinMemorReg=true;
			// throws flag for printing whatever data after loop
			break;
		default:
			break;
	}
	// cases 1-6 are one switch waiting for the current instruction opcode to turn of whatver calculation it needs to do to be sent to store 	
	//12 swtiches on the flag to be set for true for printing data after cpu is done
	
	storeI2=true;
	//send signal OK to store CI2
	}
}


void store(){
	int rhome;
	//local pointer value to set back orignal pc when it pops from memory
	if(storeI1==true){
		executeI1=false;
		executeI2=true;
		//signal that C2 can execute while C1 is storing
		//signal that C1 is not okay to execute while C1 is storing
		
	if(opcode[0] <= 6){	
			registers[finalreg[0]]=&result[0];
			prevReg= finalreg[0];
			pc++;
		//for all 3r instructions, put the results in its designated register and increment the pc for the next instruction
		// records outputted register to use for forwarding in execute
		}else if(opcode[0]==7){
			if(branch[0]<=5){
				if(result[0]==1){
					pc=pc+offset[0];
					
					fetchI1=true;
					fetchI2=false;
					decodeI2=false;
					executeI2=false;
					storeI2=false;
					// If branch is taken, then stall CI2 from processing its instruction and use IC1 to process the branch
				}else{
					pc++;
				}
			//If branch <= 5 and false, increment and CI2 will process at fetch 
			}else{
				pc=offset[0];
			//If branch is jump/call, pc is update to the address and its ready for the next instruction in fetch
			}
		}else if (opcode[0]==8){
			*registers[addregister[0]]+=offset[0];
			*registers[regtoLoS[0]]=*registers[addregister[0]];
			pc++;
			// add the offset to the contents of addressregister, then assigned the address of the register were are storing the value of the address registers
			//increment to next instruction
		}else if (opcode[0]==9){
			*registers[addregister[0]]+=offset[0];
			*registers[addregister[0]]=*registers[regtoLoS[0]];
			pc++;
			// Add the offset to the contents of the address registers, then assigned the address of addresssregister the value of the register we store 
			//increment to next instruction
		}else if(opcode[0]==10){
			if(PuOrP0[0]==1){
				sp++;
				memory[sp]=(unsigned char *)registers[pupOreg[0]];
				pc++;
				//For push, Takes value of the register and pushes into the stack, increment the pc and stack pointer
			}else if(PuOrP0[0]==2){
				registers[pupOreg[0]]=(int *)memory[sp];
				sp--;
				pc++;
				//For pop, it gets the value from the top of the stack and places it into the desginated register
				//decreses sp and increments pc for next instruction
			}else{
				rhome=atoi((const char *)memory[0]);
				pc=rhome+1;
				//For return, it gets the next instruction from the bottom of the stack and assigns the current pc  
			}
		
		}else if(opcode[0]==11){
			registers[finalreg[0]]=&immed[0];
			prevReg= finalreg[0];
			pc++;
			//For move, store immediate value into desginated register and incr pc
			//records outputted register to check with the incoming instruction
		}else{
			pc++;
			//nothing store for interrupt, increment pc
		}
	}
	if(storeI2==true){
		executeI2=false;
		executeI1=true;
		//Signal C1 is okay to execute while C2 is storing
		//singal C2 is not okay to execute while C2 is storing
		
		if(opcode[1] <= 6){	
			registers[finalreg[1]]=&result[1];
			prevReg= finalreg[1];
			pc++;
		//for all 3r instructions, put the results in its designated register and increment the pc for the next instruction
		// records outputted register to use for forwarding in execute
		}else if(opcode[1]==7){
			if(branch[1]<=5){
				if(result[1]==1){
					pc=pc+offset[1];
					
					fetchI2=true;
					fetchI1=false;
					decodeI1=false;
					executeI1=false;
					storeI1=false;
					// If branch is true, then use CI2 to do the branch instruction and stall the CI1 from processing its instruction
				}else{
					pc++;
				}
			//If branch is <=5 and false, incremenent pc and IC1 will process at fetch
			}else{
				pc=offset[1];
			//If branch is jump/call, pc is update to the address and its ready for the next instruction in fetch
			}
		}else if (opcode[1]==8){
			*registers[addregister[1]]+=offset[1];
			*registers[regtoLoS[1]]=*registers[addregister[1]];
			pc++;
			// add the offset to the contents of addressregister, then assigned the address of the register were are storing the value of the address registers
			//increment to next instruction
		}else if (opcode[1]==9){
			*registers[addregister[1]]+=offset[1];
			*registers[addregister[1]]=*registers[regtoLoS[1]];
			pc++;
			// Add the offset to the contents of the address registers, then assigned the address of addresssregister the value of the register we store 
			//increment to next instruction
		}else if(opcode[1]==10){
			if(PuOrP0[1]==1){
				sp++;
				memory[sp]=(unsigned char *)registers[pupOreg[1]];
				pc++;
				//For push, Takes value of the register and pushes into the stack, increment the pc and stack pointer
			}else if(PuOrP0[1]==2){
				registers[pupOreg[1]]=(int *)memory[sp];
				sp--;
				pc++;
			//For pop, it gets the value from the top of the stack and places it into the desginated register
			//decreses sp and increments pc for next instruction
			}else{
				rhome=atoi((const char *)memory[0]);
				pc=rhome+1;
				//For return, it gets the next instruction from the bottom of the stack and assigns the current pc  
			}
		
		}else if(opcode[1]==11){
			registers[finalreg[1]]=&immed[1];
			prevReg= finalreg[1];
			pc++;
			//For move, store immediate value into desginated register and incr pc
			// records final register to use for forawarding in execute
		}else{
			pc++;
			//nothing store for interrupt, increment pc
		}
	}
}


int main(int argc, char *argv[]){
	if (argc != 2) {
		printf ("file was not processed,there's only %d arguments\n",argc); 
		return 1;
		}
	printf("%s",argv[1]);
	load(argv[1]);
	//command line asks user for input file
	//load function fills memory with bytes from file

	while(flag!=true){
		fetch();
		decode();
		execute();
		store();
	}
	//while the pc is not at the end of memory or halt is thrown, if flag is throw then it will stop 
	// it will run fetch, decode, execute, store on each of the following instructions 

	if(prinMemorReg==true){
		if(memOrStore==0){
			for(int a=0; a<16; a++){
				printf("R%d= %d\n",a,*registers[a]);
			}
			// print out Registers 0-15 and there values
		}else{
			for(int b=1; b<sp;b++){
				printf("Memory[%d] = %d\n",b,*memory[b]);	
			}
		}	
	}
	// If there was a interupt read by the VM, after the Vm is done compiling it will print out the interrupt called
	// 0 for registers and 1 for memory 
}