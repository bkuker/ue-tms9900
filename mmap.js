export default {//UE TMS9900 Homebrew MMIO

    tape: {//TAPE DRIVE MMIO
        //ADDRESSES TBD
        tapeirq: 2,
    },
    mux0: {//MUX 0 MMIO ADDRESSES
        stat: 0xF000,		//Put THRE onto Data Bus 8, DR onto Data Bus 9
        rerd: 0xF002,		//Clear DR flag, connect Receiver Register to Data Bus[15:8]
        rx: 0xF002,		    //   Read word, byte received in 8 LSB
        thl: 0xF004,		//Load Data Bus[15:8] into Transmitter Holding Register
        tx: 0xF004,		    //   Write word, byte to send in 8 LSB
        rst: 0xF006,		//Reset the Priority Interrupt Flip Flop
        irq: 3,		        //Mux0 IRQ
    },
    mux1: {//MUX 1 MMIO ADDRESSES
        stat: 0xF008,		//Put THRE onto Data Bus 8, DR onto Data Bus 9
        rerd: 0xF00A,		//Clear DR flag, connect Receiver Register to Data Bus[15:8]
        rx: 0xF00A,		    //   Read word, byte received in 8 LSB
        thl: 0xF00C,		//Load Data Bus[15:8] into Transmitter Holding Register
        tx: 0xF00C,		    //   Write word, byte to send in 8 LSB
        rst: 0xF00E,		//Reset the Priority Interrupt Flip Flop
        irq: 4,		        //Mux1 IRQ
    },
    timer: {//TIMER MMIO ADDRESS
        rst: 0xF010,		//Reset Timer Interrupt Flip Flop
        irq: 5,		    //Timer IRQ
    },
    parallel: {//PARALLEL PORT MMIO ADDRESSES
        busy: 0xF012,		//Puts the printer busy signal onto Data Bus[8]
        prfd: 0xF014,		//Paper feed signal to feed paper through the printer
        strb: 0xF016,		//Load Data Bus[15:8] into buffer to print
    },
}
