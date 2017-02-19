/*
 * Thomas Backman, 2012-07 (made a proper library 2011-07-30, 3 weeks after initial)
 * serenity@exscape.org
 * Support for MCP49x2 (MCP4902, MCP4912, MCP4922) added 2012-08-30,
 * with a patch and testing from Jonas Gruska

 * Code license: BSD/MIT. Whatever; I *prefer* to be credited,
 * and you're *not* allowed to claim you wrote this from scratch.
 * Apart from that, do whatever you feel like. Commercial projects,
 * code modifications, etc.

 * Gaftech <gabriel@gaftech.fr> 2012-11
 * Some customizations to fit my needs

 * Pins used:
 * Arduino pin 11 (for Uno; for Mega: 51) to device SDI (pin 4) - fixed pin
 * Arduino pin 13 (for Uno; for Mega: 52) to device SCK (pin 3) - fixed pin
 * Any digital pin to device LDAC (DAC pin 5)  (except with PortWrite, see README)
 * Any digital pin to device CS   (DAC pin 2)  (as above)
 *
 * Other DAC wirings:
 * Pin 1: VDD, to +5 V
 * Pin 5: LDAC, either to an Arduino pin, or ground to update vout automatically
 * Pin 6: VREF, to +5 V (or some other reference voltage 0 < VREF <= VDD)
 * Pin 7: VSS, to ground
 * Pin 8: vout
 * (Pin 9, for the DFN package only: VSS)

 * Only tested on MCP4901 (8-bit) and MCP4922 (dual 12-bit), but it should work on the others as well.
 * Tested on an Arduino Uno R3.
 */

#ifndef _DAC_MCP4X_H
#define _DAC_MCP4X_H

#include <Arduino.h>
#include <inttypes.h>

//
// Params
//
#define MCP4X_PORT_WRITE 1
#define MCP4X_NO_LDAC			-1
#ifndef MCP4X_PORT_WRITE
#define MCP4X_PORT_WRITE		0
#endif
#ifndef MCP4X_DEFAULTS
#define MCP4X_DEFAULTS			(MCP4X_BUFFERED | MCP4X_GAIN_1X | MCP4X_ACTIVE)
#endif

//
// Constants
//

// Bits
#define MCP4X_WRITE_B			(1 << 15)
#define MCP4X_BUFFERED			(1 << 14)
#define MCP4X_GAIN_1X			(1 << 13)
#define MCP4X_ACTIVE			(1 << 12)

// Channels
#define MCP4X_CHAN_A			0
#define MCP4X_CHAN_B			1

// Model identifiers
#define MCP4X_8_BITS			0
#define MCP4X_10_BITS			1
#define MCP4X_12_BITS			2
#define MCP4X_INTREF			(1 << 2)
#define MCP4X_DUAL				(1 << 3)

// Model list (each model coded on 4 bits)
#define MCP4X_4801					(MCP4X_8_BITS 	| MCP4X_INTREF)
#define MCP4X_4811					(MCP4X_10_BITS 	| MCP4X_INTREF)
#define MCP4X_4821					(MCP4X_12_BITS 	| MCP4X_INTREF)
#define MCP4X_4802					(MCP4X_8_BITS 	| MCP4X_INTREF | MCP4X_DUAL)
#define MCP4X_4812					(MCP4X_10_BITS 	| MCP4X_INTREF | MCP4X_DUAL)
#define MCP4X_4822					(MCP4X_12_BITS 	| MCP4X_INTREF | MCP4X_DUAL)
#define MCP4X_4901					(MCP4X_8_BITS)
#define MCP4X_4911					(MCP4X_10_BITS)
#define MCP4X_4921					(MCP4X_12_BITS)
#define MCP4X_4902					(MCP4X_8_BITS 	| MCP4X_DUAL)
#define MCP4X_4912					(MCP4X_10_BITS 	| MCP4X_DUAL)
#define MCP4X_4922					(MCP4X_12_BITS 	| MCP4X_DUAL)

class MCP4X {
public:

	byte init(byte model,
			unsigned int vrefA = 5000, unsigned int vrefB = 5000,
			int ss_pin = SS, int ldac_pin = MCP4X_NO_LDAC, boolean autoLatch = 1);
	void begin(boolean beginSPI = 1);

	void configureSPI();

	void setVref(byte chan, unsigned int vref)			{ vrefs[chan & 1] = vref;		}
	void setVref(unsigned int vref)						{ vrefs[0] = vrefs[1] = vref;	}
	void setBuffer(byte chan, boolean buffered);
	void setGain2x(byte chan, boolean gain2x = 1);
	void setAutoLatch(boolean enabled = 1)				{ autoLatch = enabled;			}
	void shutdown(byte chan, boolean off = 1);

	void output(byte _chan, unsigned short _out);
	void output(unsigned short data) 					{ output(MCP4X_CHAN_A, data); 	}
	;
	/* same as output(), but having A/B makes more sense for dual DACs */
	void outputA(unsigned short data)					{ output(MCP4X_CHAN_A, data); 	}
	void outputB(unsigned short data) 					{ output(MCP4X_CHAN_B, data);	}
	void output2(unsigned short _out, unsigned short _out2); // For MCP49x2

	//
	// output-like methods that takes a voltage argument
	//
	void setVoltage(byte chan, float v);

	int getGain(byte chan)						{ return regs[chan & 1] & MCP4X_GAIN_1X ? 1 : 2;	}
	float getVoltageMV(byte chan);


	/* Actually change the output, if the LDAC pin isn't shorted to ground */
	void latch(void);

	/* Only relevant for the MCP49x2 dual DACs.
	 * If set, calling output2() will pull the LDAC pin low automatically,
	 * which causes the output to change.
	 * Not required if the LDAC pin is shorted to ground, however in that case,
	 * there will be a delay between the updating of channel A and channel B.
	 * If sync is desired, wire the LDAC pin to the Arduino and set this to true.
	 */
	void setAutomaticallyLatchDual(bool latch)	{ autoLatch = latch;	};

private:
	unsigned int vrefs[2];
	unsigned int regs[2];
	boolean dual;
	int ss_pin;
	int LDAC_pin;
	int bitwidth;
	boolean autoLatch; /* call latch() automatically after output2() has been called? */

	void write(unsigned int data);
};

#endif
