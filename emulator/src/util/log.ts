export enum LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    NONE = 4
}

export class Log {

    private static readonly BUFFER_SIZE = 32;
    private static readonly MAX_LOG_LINES = 4096;

    private static instance: Log;

    private minLevel = LogLevel.INFO;
    private buffer: string[] = [];
    private sameMessageCount = 0;
    private totalLines = 0;

    static getLog(): Log {
        if (!this.instance) {
            this.instance = new Log();
        }
        return this.instance;
    }

    constructor() {
        setInterval(() => {
            this.flushBuffer();
        }, 1000);
    }


    clear() {
        this.totalLines = 0;
    }

    print(message: string) {
        if (this.buffer.length === 0 || this.buffer[this.buffer.length - 1] !== message) {
            this.updateSameMessage();
            this.buffer.push(message);
        } else {
            this.sameMessageCount++;
        }
        if (this.buffer.length > Log.BUFFER_SIZE) {
            this.flushBuffer();
        }
    }

    flushBuffer() {
        if (this.buffer.length) {
            this.updateSameMessage();
            const messages = this.buffer.join('\n') + (this.buffer.length ? '\n' : '');
            console.log(this.buffer);
            this.buffer = [];
        }
    }

    updateSameMessage() {
        if (this.sameMessageCount > 0) {
            this.buffer[this.buffer.length - 1] += " (" + (this.sameMessageCount + 1) + ")";
            this.sameMessageCount = 0;
        }
    }

    /**
     * Set minimum log level.
     * @param level Log level to set
     */
    setMinLevel(level: LogLevel) {
        this.minLevel = level;
    }

    /**
     * Log error message.
     * @param message error message
     */
    error(message: string) {
        if (LogLevel.ERROR >= this.minLevel) {
            console.error(message);
        }
    }

    /**
     * Log warning message.
     * @param message warning message
     */
    warn(message: string) {
        if (LogLevel.WARNING >= this.minLevel) {
            this.print('*** Warning *** ' + message);
        }
    }

    /**
     * Log information message.
     * @param message information message
     */
    info(message: string) {
        if (LogLevel.INFO >= this.minLevel) {
            this.print(message);
        }
    }

    /**
     * Log debug message.
     * @param message debug message
     */
    debug(message: string) {
        if (LogLevel.DEBUG >= this.minLevel) {
            console.debug(message);
        }
    }
}
