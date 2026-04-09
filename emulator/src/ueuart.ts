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

const StatTxReadyToSend =  0b00000001_00000000;
const StatRXDataReady = 0b00000010_00000000;

export class UeUart implements InterruptSource, MemoryMapped {
    private log: Log = Log.getLog();

    private rData: number;
    private rReady: boolean;

    private tData: number;
    private tReady: boolean;
    private intFF: boolean;

    private muxInfo;

    private byteConsumer: (byte: number) => void;

    constructor(muxInfo, intEnc: InterruptEncoder) {
        this.muxInfo = muxInfo;

        this.rData = 64;
        this.rReady = false;
        this.tData = 0;
        this.tReady = true;
        this.intFF = false;

        intEnc.registerInterruptSource(this);

        this.byteConsumer = (b) => console.log(`Serial byte ${b}`);
    }

    public offerByteFromTerminal(byte: number): boolean {
        if (this.rReady)
            return false;
        this.rData = byte;
        this.rReady = true;
        this.intFF = true;
        return true;
    }

    public setTerminalByteConsumer(f: (byte: number) => void): void {
        this.byteConsumer = f;
    }

    public getBaseAddress() {
        return this.muxInfo.stat;
    };

    public getSize() {
        return 8;
    }

    getInterruptCode(): number {
        return this.muxInfo.irq;
    }

    isAssertingInterrupt(): boolean {
        return this.intFF;
    }

    public readWord(addr: number): number {
        if (addr == this.muxInfo.stat) {
            //Read status: bit 0 =  Transmit Busy, bit 1 = Data Received
            return (this.tReady ? StatTxReadyToSend : 0) | (this.rReady ? StatRXDataReady : 0);
        } else if (addr == this.muxInfo.rerd ) {
            //RRD Read the data
            if (!this.rReady)
                this.log.warn("UART read when no data ready");
            this.rReady = false;
            return this.rData << 8;
        } else {
            //Not weird for RMW for MOVB
            //this.log.warn("Weird UART read from address 0x" + addr.toString(16));
        }
    }

    public writeWord(addr: number, w: number) {
        if (addr == this.muxInfo.thl) {
            //THRL, load data to send
            this.tData = w >> 8;
            //console.log("UART Write 0x" +this.tData.toString(16) + " " + String.fromCharCode(this.tData));
            setTimeout(() => {
                this.byteConsumer(this.tData);
                this.tReady = true;
                this.intFF = true;
            }, 4);
            this.tReady = false;
        } else if (addr == this.muxInfo.rst ) {
            this.intFF = false;
        } else {
            this.log.warn("Weird UART write " + w + " to address 0x" + addr.toString(16));
        }
    }

}