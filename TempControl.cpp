/*
 * TempControl.cpp
 *
 *  Created on: Sep 14, 2016
 *      Author: kghome
 */

#include <Arduino.h>
#include <TempControl.h>
#include <math.h>


TempControl::TempControl(Print &debug, int pin) : log(debug),irsend(pin) {

	worktime = 0;
	average = 0;
	first_time = 1;
}

TempControl::~TempControl() {
	// TODO Auto-generated destructor stub
}

void TempControl::reset()
{
	curstate = RESTART;
}
void TempControl::setstate(enum FANSTATE state, int curtemp)
{
	change_time =  millis();

	if (state != curstate) {
		// Timing sensitive, so disable interrupts for transmission
		noInterrupts();
		switch(state) {
		case RESTART:
			break;  // should not get here
		case OVERTEMP_SPEED:
			irsend.sendHvacMitsubishi(HVAC_CONTROL::HVAC_COLD, 15, HVAC_CONTROL::FAN_SPEED_5, HVAC_CONTROL::VANNE_AUTO_MOVE, false);
			break;
		case HIGH_SPEED:
			irsend.sendHvacMitsubishi(HVAC_CONTROL::HVAC_COLD, 15, HVAC_CONTROL::FAN_SPEED_4, HVAC_CONTROL::VANNE_H1, false);
			break;
		case MED_SPEED:
			irsend.sendHvacMitsubishi(HVAC_CONTROL::HVAC_COLD, 15, HVAC_CONTROL::FAN_SPEED_3, HVAC_CONTROL::VANNE_H1, false);
			break;
		case NORMAL_SPEED:
			irsend.sendHvacMitsubishi(HVAC_CONTROL::HVAC_COLD, 15, HVAC_CONTROL::FAN_SPEED_2, HVAC_CONTROL::VANNE_H1, false);
			break;
		case LOW_SPEED:
			irsend.sendHvacMitsubishi(HVAC_CONTROL::HVAC_COLD, 15, HVAC_CONTROL::FAN_SPEED_1, HVAC_CONTROL::VANNE_H1, false);
			break;
		case OFF:
			irsend.sendHvacMitsubishi(HVAC_CONTROL::HVAC_COLD, 33, HVAC_CONTROL::FAN_SPEED_AUTO, HVAC_CONTROL::VANNE_AUTO_MOVE, true);
			break;
		}
		interrupts();
	}
	curstate = state;
}
double TempControl::getSlope(void)
{
	return slope;
}
void TempControl::calcSlope(void)
{
	// find the slope of the temperature over the last n minutes
	//
	//  nExy - ExEy
	// -------------
	//     2        2
	//  nEx   - (Ex)
    float nr=0,dr=0;
    float y;
    int it = cur_slot;

    double sum_y=0,sum_xy=0,sum_x=0,sum_xx=0,sum_x2=0,intercept=0,reg;
    int i,n=SLOTS;
    for(i=0;i<n;i++)
    {
    	y = lasttemp[it];
    	it = (it + 1) % SLOTS;
    	sum_x+=i;
    	sum_y+=y;
    	sum_xx+= i*i;
    	sum_xy+= i*y;
    }
    nr=(n*sum_xy)-(sum_x*sum_y);
    sum_x2=sum_x*sum_x;
    dr=(n*sum_xx)-sum_x2;
    slope=nr/dr;
    double fac = pow(10, 2);
    slope=round(slope*fac)/fac;

}

float TempControl::updateTemp(float ctemp, int humidity)
{
	int i;
	unsigned long now = millis();
	// seconds since we changed the fan settings
	unsigned long lastchange_seconds = ((now - change_time) + 500) / 1000;
#define CHANGE_LIMIT (60*5) // minimum of 3 minutes between changes to allow average to work
#define STUCK_LIMIT (60*60) // maximum run time if no change

	bool change_allowed = lastchange_seconds > CHANGE_LIMIT;
	bool long_run = lastchange_seconds > STUCK_LIMIT;

	if (curstate == RESTART) change_allowed = true;

	if (first_time) {
		for (i=0;i<SLOTS;++i)
			lasttemp[i] = ctemp;
		first_time=0;
	}

	lasttemp[cur_slot] = ctemp;
	cur_slot = (cur_slot + 1) % SLOTS;
	average = 0;
	for (i=0;i<SLOTS;++i)
		average += lasttemp[i];

	float curtemp = average/SLOTS;
	float stdev = 0;

	for (i=0;i<SLOTS;++i) {
		stdev += (lasttemp[i]-curtemp)*(lasttemp[i]-curtemp);
	}
	stdev = stdev / SLOTS;
	stdev = sqrt(stdev);

	log.print(F("avg:"));
	log.print(curtemp);
	log.print(F(" stdev:"));
	log.print(stdev);
	log.print(F(" set:"));
	log.print(desiredtemp);
	log.print(F(" since:"));
	log.print(lastchange_seconds);
	log.print(F(" "));

	calcSlope();

    // if the temp is going up, then we need more cooling
    // if the temp is high, then we need alot!

	// diff negative means too hot
	// diff positve too cold
	float diff = desiredtemp - curtemp;
	float swing;
	if (worktime) {
		swing = .5;
	} else {
		swing = 1; // 2degree swing on off work times...
		//change_allowed = false;
	}
	log.print(" diff: ");
	log.print(diff,2);
	log.print(" slope: ");
	log.print(slope,2);
	log.print(" swing: ");
	log.print(swing,2);

	if (diff > swing) {
		log.print(F("OFF-COLD"));
		setstate(OFF,desiredtemp);
	} else
	// cold, decrease cooling
	if (diff >= 0) {
		// if we are under temp, then slowly back off
		if (diff > swing && slope <= 0 && change_allowed ) {
			setstate(OFF,desiredtemp);
		} else
		if (slope <= 0 && curstate > LOW_SPEED && change_allowed ) {
			setstate((FANSTATE)((int)curstate-1),desiredtemp);
		} else
		// avoid being in same speed for long time
		if (slope <= 0.1 && curstate > OFF && long_run ) {
			setstate((FANSTATE)((int)curstate-1),desiredtemp);
		}
	} else
	// hot, increase cooling
	if (diff < -swing*2) {
		// if we are way off..  then something is wrong, keep trying to set the state
		// by forcing the state to off.
		setstate(OVERTEMP_SPEED,desiredtemp);
	} else {
		// if we are over temp, then slowly increase fan speed until we are cool again
		if (curstate <= HIGH_SPEED && slope > 0 && change_allowed ) {
			setstate((FANSTATE)((int)curstate+1),desiredtemp);
		} else
		// avoid being in same speed for long time with a small slope
		if (curstate < HIGH_SPEED && slope > -.2 && long_run ) {
			setstate((FANSTATE)((int)curstate+1),desiredtemp);
		}
	}
	log.print(" state: ");
	log.print(curstate);
    log.println(" - end");
	return desiredtemp;
}
float TempControl::get_curtemp(void) {
        float average;
	int i;
	average = 0;
	for (i=0;i<SLOTS;++i)
		average += lasttemp[i];

	float curtemp = average/SLOTS;
   	return curtemp;
	float stdev = 0;

	for (i=0;i<SLOTS;++i) {
		stdev += (lasttemp[i]-curtemp)*(lasttemp[i]-curtemp);
	}
	stdev = stdev / SLOTS;
	stdev = sqrt(stdev);
}
