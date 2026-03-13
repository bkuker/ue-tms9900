/**
 * Emulates the UART card from the UE Homebrew TMS990 computer
 * 
 * 
 * THRE    EQU >F000 <- Read, byte 1 means ready to transmit
 * THRL    EQU >F002 -> Write data to send
 * RRD     EQU >F004 <- Read data received
 * DRR     EQU >F006 -> Write to reset data received signal
 * 
 * M0STAT  EQU >F000    ; Read status: bit 0 = Transmit Busy, bit 1 = Data Received
 * M0RX    EQU >F002    ; Read byte received in 8 LSB
 * M0TX    EXU >F004    ; Write byte to send in 8 LSB
 * M0RST   EQU >F006    ; Write to reset interrupt and Data Received status
 */

import { Log } from './util/log';
import { InterruptEncoder, InterruptSource } from './InterruptEncoder';
import { MemoryMapped } from './memory';

//Offsets from baseaddr
const MxSTAT = 0;
const MxRX = 2;
const MxTX = 4;
const MxRST = 6;

const StatTxBusy =  0b0000000000000001;
const StatRXEmpty = 0b0000000000000010;

export class UeUart implements InterruptSource, MemoryMapped {
    private log: Log = Log.getLog();

    private rData: number;
    private rReady: boolean;

    private tData: number;
    private tReady: boolean;
    private tReadyInterrupt: boolean;

    private baseAddr: number;
    private irq: number;

    private byteConsumer: (byte: number) => void;

    constructor(baseAddr: number, irq: number, intEnc: InterruptEncoder) {
        this.baseAddr = baseAddr;
        this.irq = irq;

        this.rData = 64;
        this.rReady = false;
        this.tData = 0;
        this.tReady = true;
        this.tReadyInterrupt = false;


        intEnc.registerInterruptSource(this);

        this.byteConsumer = (b) => console.log(`Serial byte ${b}`);
    }

    public offerByteFromTerminal(byte: number): boolean {
        if (this.rReady)
            return false;
        this.rData = byte;
        this.rReady = true;
        return true;
    }

    public setTerminalByteConsumer(f: (byte: number) => void): void {
        this.byteConsumer = f;
    }

    public getBaseAddress() {
        return this.baseAddr;
    };

    public getSize() {
        return 8;
    }

    getInterruptCode(): number {
        return this.irq;
    }

    isAssertingInterrupt(): boolean {
        return this.rReady || this.tReadyInterrupt;
    }

    public readWord(addr: number): number {
        if (addr == this.baseAddr + MxSTAT) {
            //Read status: bit 0 =  Transmit Busy, bit 1 = Data Received
            return (this.tReady ? 0 : StatTxBusy) | (this.rReady ? 0 : StatRXEmpty);
        } else if (addr == this.baseAddr + MxRX) {
            //RRD Read the data
            if (!this.rReady)
                this.log.warn("UART read when no data ready");
            return this.rData;
        } else {
            //Not weird for RMW for MOVB
            //this.log.warn("Weird UART read from address 0x" + addr.toString(16));
        }
    }

    public writeWord(addr: number, w: number) {
        if (addr == this.baseAddr + MxTX) {
            //THRL, load data to send
            this.tData = w & 0xFF;
            //console.log("UART Write 0x" +this.tData.toString(16) + " " + String.fromCharCode(this.tData));
            setTimeout(() => {
                this.byteConsumer(this.tData);
                this.tReady = true;
                this.tReadyInterrupt = true;
            }, 4);
            this.tReady = false;
        } else if (addr == this.baseAddr + MxRST) {
            //DRR, reset data ready
            //console.log("Clear DDR")
            this.rReady = false;
            this.tReadyInterrupt = false;
        } else {
            this.log.warn("Weird UART write " + w + " to address 0x" + addr.toString(16));
        }
    }

}