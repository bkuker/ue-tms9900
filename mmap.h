//UE TMS9900 Homebrew MMIO

//TAPE DRIVE MMIO
//ADDRESSES TBD
#define TAPEIRQ	2

//MUX 0 MMIO ADDRESSES
#define M0STAT	0xF000		//Put THRE onto Data Bus 8, DR onto Data Bus 9
#define M0RERD	0xF002		//Clear DR flag, connect Receiver Register to Data Bus[15:8]
#define M0RX	0xF002		//   Read word, byte received in 8 LSB
#define M0THL	0xF004		//Load Data Bus[15:8] into Transmitter Holding Register
#define M0TX	0xF004		//   Write word, byte to send in 8 LSB
#define M0RST	0xF006		//Reset the Priority Interrupt Flip Flop
#define M0IRQ	3	    	//Mux0 IRQ

//MUX 1 MMIO ADDRESSES
#define M1STAT	0xF008		//Put THRE onto Data Bus 8, DR onto Data Bus 9
#define M1RERD	0xF00A		//Clear DR flag, connect Receiver Register to Data Bus[15:8]
#define M1RX	0xF00A		//   Read word, byte received in 8 LSB
#define M1THL	0xF00C		//Load Data Bus[15:8] into Transmitter Holding Register
#define M1TX	0xF00C		//   Write word, byte to send in 8 LSB
#define M1RST	0xF00E		//Reset the Priority Interrupt Flip Flop
#define M1IRQ	4	    	//Mux1 IRQ

//TIMER MMIO ADDRESS
#define TIRST	0xF010		//Reset Timer Interrupt Flip Flop
#define TMRIRQ	5		    //Timer IRQ

//PARALLEL PORT MMIO ADDRESSES
#define PRBUSY	0xF012		//Puts the printer busy signal onto Data Bus[8]
#define PPRFD	0xF014		//Paper feed signal to feed paper through the printer
#define PRSTRB	0xF016		//Load Data Bus[15:8] into buffer to print
