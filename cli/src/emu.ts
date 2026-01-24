import * as fs from 'fs';
import { TMS9900 } from '@ue-tms9900/emulator/tms9900';
import { UeTMS990 } from '@ue-tms9900/emulator/UeTMS990';
import { Command } from 'commander';

//Get Command Line
const commandLine = new Command();
commandLine
    .argument('<rom file name>');
commandLine.parse();

main().catch((err) => {
    console.error(err);
    process.exit(1);
});


async function main(): Promise<void> {
    const rom = commandLine.args[0]
    console.log(`Loading ${rom} @0x0000`);
    const buffer = fs.readFileSync(rom);
    let romContents = new Uint8Array(buffer);

    let ue = new UeTMS990(romContents);

    process.stdin.setRawMode(true);
    process.stdin.resume();
    process.stdin.setEncoding('utf8');
    process.stdin.on("data", (key: string) => {
        if (key === "\u0003")
            process.exit(); // Ctrl+C
        else
            ue.mux0.offerByteFromTerminal(key.charCodeAt(0));
    });
    ue.mux0.setTerminalByteConsumer(b => process.stdout.write(String.fromCharCode(b)));

    ue.cpu.reset();

    //Run the CPU Continuously...
    while (true) {
        ue.cpu.run(1000, false);
        //But yeild every so often so UARTs and other things 
        //do their thing
        await new Promise(r => setImmediate(r));
    }
}