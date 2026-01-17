/*
TR1602B
SFD Status Flag Disconect allows reading of THRE

RRC & TRC Baud Rate, set to 300

THRE Transmitter Holding Register Empty - lets us know when uart is done transmitting
TRE Transmitter Register Empty ignore

THRL Transmitter Holding Register Load - strobe and uart grabs value off data bus

8 bit word length

ROM for.... memory map io logic

RRD Receive Register Disconnect puts outputs on data bus

DRR Date Received Reset resets DR
DR Data Received - off to the IRQ Level 8, b1000
RI Receiver Input


THRE    EQU >F000 <- Read, byte 1 means ready to transmit
THRL    EQU >F002 -> Write data to send
RRD     EQU >F004 <- Read data received
DRR     EQU >F006 -> Write to reset data received signal



*/
import { Log } from '../../classes/log';

export class UeUart {
    private log: Log = Log.getLog();

    private rData: number;
    private rReady: boolean;

    private tData: number;
    private tReady: boolean;

    private baseAddr;

    constructor(baseAddr: number) {
        this.baseAddr = baseAddr;
        this.rData = 0;
        this.rReady = false;
        this.tData = 0;
        this.tReady = true;

        setInterval(() => {
            //Ready to transmit every 4 ms
            //a little slower than 300 baud
            this.tReady = true;
        }, 4);
    }

    public readWord(addr: number): number {
        if (addr == this.baseAddr) {
            //THRE Transmitter Holding Register Empty
            //  Is it free to accept another byte to send?
            return this.tReady ? 0 : 1;
        } else if (addr == this.baseAddr + 4) {
            //RRD Read the data
            if (!this.rReady)
                this.log.warn("UARD read when no data ready")
            return this.rData;
        } else {
            this.log.warn("UARD read from address " + addr);
        }
    }

    public writeWord(addr: number, w: number) {
        if (addr = this.baseAddr + 2) {
            //THRL, load data to send
            this.tData = w & 0xFF;
            console.log("Uart Write " + String.fromCharCode(this.tData));
            this.tReady = false;
        } else if (addr = this.baseAddr + 6) {
            //DRR, reset data ready
            this.rReady = false;
        }
    }

}