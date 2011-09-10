#include "Motion.h"

#include "config.h"
#include "GCode.h"
#include "Axis.h"
#include "Globals.h"
#include "GcodeQueue.h"
#include "ArduinoMap.h"
#include <avr/pgmspace.h>



#define max(x,y) (x > y ? x : y)
#define min(x,y) (x < y ? x : y)

// Testing shows this is probably better put nearer 1000.
#define MIN_INTERVAL 500

Point& Motion::getCurrentPosition()
{
  static Point p;
  for(int ax=0;ax<NUM_AXES;ax++)
    p[ax] = AXES[ax].getCurrentPosition();
  return p;
}

void Motion::setCurrentPosition(GCode &gcode)
{
  for(int ax=0;ax<NUM_AXES;ax++)
  {
    if(!gcode[ax].isUnused())
    {
      AXES[ax].setCurrentPosition(gcode[ax].getFloat());
    }
  }
}

void Motion::setAbsolute()
{
  for(int ax=0;ax<NUM_AXES;ax++)
    AXES[ax].setAbsolute();
}

void Motion::setRelative()
{
  for(int ax=0;ax<NUM_AXES;ax++)
    AXES[ax].setRelative();
}

void Motion::setMinimumFeedrate(GCode& gcode)
{
  for(int ax=0;ax < NUM_AXES;ax++)
  {
    if(gcode[ax].isUnused()) continue;
    AXES[ax].setMinimumFeedrate(gcode[ax].getFloat());
  }
}

void Motion::setMaximumFeedrate(GCode& gcode)
{
  for(int ax=0;ax < NUM_AXES;ax++)
  {
    if(gcode[ax].isUnused()) continue;
    AXES[ax].setMaximumFeedrate(gcode[ax].getFloat());
  }
}

void Motion::setAverageFeedrate(GCode& gcode)
{
  for(int ax=0;ax < NUM_AXES;ax++)
  {
    if(gcode[ax].isUnused()) continue;
    AXES[ax].setAverageFeedrate(gcode[ax].getFloat());
  }
}

void Motion::setStepsPerUnit(GCode& gcode)
{
  for(int ax=0;ax < NUM_AXES;ax++)
  {
    if(gcode[ax].isUnused()) continue;
    AXES[ax].setStepsPerUnit(gcode[ax].getFloat());
  }
}

void Motion::setAccel(GCode& gcode)
{
  for(int ax=0;ax < NUM_AXES;ax++)
  {
    if(gcode[ax].isUnused()) continue;
    AXES[ax].setAccel(gcode[ax].getFloat());
  }
}

#define MOT_CHANGEPIN(FOO) \
  for(int ax=0;ax < NUM_AXES;ax++) \
  { \
    if(gcode[ax].isUnused()) \
      continue; \
    \
    AXES[ax].FOO(ArduinoMap::getPort(gcode[ax].getInt()), ArduinoMap::getPinnum(gcode[ax].getInt())); \
  } 
 

void Motion::setStepPins(GCode& gcode) { MOT_CHANGEPIN(changepinStep); }
void Motion::setDirPins(GCode& gcode) { MOT_CHANGEPIN(changepinDir); }
void Motion::setEnablePins(GCode& gcode) { MOT_CHANGEPIN(changepinEnable); }
void Motion::setMinPins(GCode& gcode) { MOT_CHANGEPIN(changepinMin); }
void Motion::setMaxPins(GCode& gcode) { MOT_CHANGEPIN(changepinMax); } 
void Motion::setAxisInvert(GCode& gcode)
{
  for(int ax=0;ax < NUM_AXES;ax++)
  {
    if(gcode[ax].isUnused()) continue;
    AXES[ax].setInvert(gcode[ax].getInt());
  }
}

void Motion::setMinStopPos(GCode& gcode)
{
  for(int ax=0;ax < NUM_AXES;ax++)
  {
    if(gcode[ax].isUnused()) continue;
    AXES[ax].setMinStopPos(gcode[ax].getFloat());
  }
}

void Motion::setMaxStopPos(GCode& gcode)
{
  for(int ax=0;ax < NUM_AXES;ax++)
  {
    if(gcode[ax].isUnused()) continue;
    AXES[ax].setMaxStopPos(gcode[ax].getFloat());
  }
}




void Motion::setAxisDisable(GCode& gcode)
{
  for(int ax=0;ax < NUM_AXES;ax++)
  {
    if(gcode[ax].isUnused()) continue;
    AXES[ax].setDisable(gcode[ax].getInt());
  }
}

void Motion::setEndstopGlobals(bool inverted, bool pulledup)
{
  Axis::setPULLUPS(pulledup);
  Axis::setEND_INVERT(inverted);
}

void Motion::reportConfigStatus(Host& h)
{
  for(int ax=0;ax < NUM_AXES;ax++)
  {
    h.labelnum("CS:", ax, false);
    h.write(' ');
    AXES[ax].reportConfigStatus(h);
    h.endl();
  }
}


void Motion::disableAllMotors()
{
  for(int ax=0;ax < NUM_AXES;ax++)
    AXES[ax].disable();
}

void Motion::checkdisable(GCode& gcode)
{
  // This was STUPID
}

void Motion::disableAxis(int axis)
{
  AXES[axis].disableIfConfigured();
}




void Motion::getMovesteps(GCode& gcode)
{
  for(int ax=0;ax < NUM_AXES;ax++)
  {
    gcode.axismovesteps[ax] = AXES[ax].getMovesteps(gcode.startpos[ax], 
                                                             gcode[ax].isUnused() ? gcode.startpos[ax] : AXES[ax].isRelative() ? gcode.startpos[ax] + gcode[ax].getFloat() : gcode[ax].getFloat(), 
                                                             gcode.axisdirs[ax]);
    if(gcode.movesteps < gcode.axismovesteps[ax])
    {
      gcode.movesteps = gcode.axismovesteps[ax];
      gcode.leading_axis = ax;
    }
  }
}

float Motion::getSmallestStartFeed(GCode& gcode)
{
  float mi = 9999999;
  for(int ax=0;ax < NUM_AXES;ax++)
  {
    if(gcode.axismovesteps[ax] == 0) continue;
    float t = AXES[ax].getStartFeed(gcode.feed);
    if(t < mi) mi = t;
  }
  return mi;
}

float Motion::getSmallestEndFeed(GCode& gcode)
{
  float mi = 9999999;
  for(int ax=0;ax < NUM_AXES;ax++)
  {
    if(gcode.axismovesteps[ax] == 0) continue;
    float t = AXES[ax].getEndFeed(gcode.feed);
    if(t < mi) mi = t;
  }
  return mi;
}

void Motion::getActualEndpos(GCode& gcode)
{
  for(int ax=0;ax<NUM_AXES;ax++)
  {
    gcode.endpos[ax] = AXES[ax].getEndpos(gcode.startpos[ax], gcode.axismovesteps[ax], gcode.axisdirs[ax]);
  }
}


void Motion::gcode_precalc(GCode& gcode, float& feedin, Point* lastend)
{
  if(gcode.state >= GCode::PREPARED)
    return;

  if(gcode[G].isUnused())
    return;

  if(gcode[G].getInt() == 92)
  {
    // Just carry over new position
    for(int ax=0;ax<NUM_AXES;ax++)
    {
      if(!gcode[ax].isUnused())
      {
        (*lastend)[ax] = gcode[ax].getFloat();
      }
    }
    gcode.state = GCode::PREPARED;
    return;
  }
  if(gcode[G].getInt() != 1 and gcode[G].getInt() != 2)
  {
    // no precalc for anything but 92, 1, 2
    gcode.state = GCode::PREPARED;
    return;
  }

  // We want to carry over the previous ending position and feedrate if possible.
  gcode.startpos = *lastend;
  if(gcode[F].isUnused())
    gcode.feed = feedin;
  else
    gcode.feed = gcode[F].getFloat();

  if(gcode.feed == 0)
    gcode.feed = SAFE_DEFAULT_FEED;


  getMovesteps(gcode);
  getActualEndpos(gcode);

  *lastend = gcode.endpos;
  feedin  = gcode.feed;
  gcode.state = GCode::PREPARED;

  if(gcode.movesteps == 0)
    return;

  // Calculate actual distance of move along vector
  gcode.actualmm  = 0;
  for(int ax=0;ax<NUM_AXES;ax++)
  {
    gcode.actualmm += pow(gcode.axismovesteps[ax]/AXES[ax].getStepsPerMM(), 2);
  }
  gcode.actualmm = sqrt(gcode.actualmm);

  float axisspeeds[NUM_AXES];
  // Calculate individual axis movement speeds
  float mult = gcode.feed/gcode.actualmm;
  for(int ax=0;ax<NUM_AXES;ax++)
  {
    axisspeeds[ax] = (gcode.axismovesteps[ax] / AXES[ax].getStepsPerMM()) * mult;
#ifdef DEBUG_LAME
    HOST.labelnum("AS", ax, false);
    HOST.labelnum(":", axisspeeds[ax]);
#endif    
  }

  // Calculate ratio of movement speeds to main axis
  for(int ax=0;ax < NUM_AXES;ax++)
  {
    if(axisspeeds[ax] != 0)
      gcode.axisratio[ax] = axisspeeds[gcode.leading_axis] / axisspeeds[ax];
    else
      gcode.axisratio[ax] = 0;
  }

  // Check all axis for violation o top speed.  Take the biggest violator and change
  // the leading axis speed to match it.
  float bigdiff = 0;
  float littlediff = 0;
  for(int ax = 0;ax<NUM_AXES;ax++)
  {
    if(axisspeeds[ax] > AXES[ax].getMaxFeed())
    {
      float d = (axisspeeds[ax] - AXES[ax].getMaxFeed()) * gcode.axisratio[ax];
#ifdef DEBUG_LAME
      HOST.labelnum("D",ax,false);
      HOST.labelnum(":",axisspeeds[ax] - AXES[ax].getMaxFeed(),false);
      HOST.labelnum(" rat: ", gcode.axisratio[ax]);
#endif      


      if(d > bigdiff)
        bigdiff = d;
    }

    if(axisspeeds[ax] > AXES[ax].getStartFeed())
    {
      float d = (axisspeeds[ax] - AXES[ax].getStartFeed()) * gcode.axisratio[ax];
      if(d > littlediff)
        littlediff = d;
    }
  }
#ifdef DEBUG_LAME
  HOST.labelnum("BD:", bigdiff);
  HOST.labelnum("LD:", littlediff);
#endif

  gcode.maxfeed = axisspeeds[gcode.leading_axis] - bigdiff;
  gcode.startfeed = axisspeeds[gcode.leading_axis] - littlediff;
  gcode.endfeed   = gcode.startfeed;
  gcode.currentfeed = gcode.startfeed;

  // TODO: We shoudl treat accel as we do the speeds above and scale it to fit the chosen axis. 
  // instead we just take the accel of the leadng axis.
  float accel = AXES[gcode.leading_axis].getAccel();
  gcode.accel = accel;


#ifdef DEBUG_LAME
 HOST.labelnum("F1: ", gcode.startfeed);
 HOST.labelnum("F2: ", gcode.maxfeed);
 HOST.labelnum("Accel: ", accel);
#endif


  uint32_t dist = AXES[gcode.leading_axis].getAccelDist(gcode.startfeed, gcode.maxfeed, accel);
  uint32_t halfmove = gcode.movesteps >> 1;
  if(halfmove <= dist)
  {
    gcode.accel_until = gcode.movesteps - halfmove;
    gcode.decel_from  = halfmove;
  }
  else
  {
    gcode.accel_until =  gcode.movesteps - dist;
    gcode.decel_from  =  dist;
  }

  gcode.currentinterval = AXES[gcode.leading_axis].int_interval_from_feedrate(gcode.startfeed);
  gcode.currentfeed = gcode.startfeed;

  // TODO: this only changes when we change accel rates; can we just store it per-axis until we scale accels properly?
  gcode.accel_inc   = (float)((float)accel * 60.0f / ACCELS_PER_SECOND);
  gcode.accel_timer = ACCEL_INC_TIME;

  gcode.optimized = false;

}

void Motion::gcode_optimize(GCode& gcode, GCode& nextg)
{
#ifdef LOOKAHEAD
  // I need to think about this a lot more.
  if(gcode.optimized)
    return;

  gcode.optimized = true;

  if(gcode[G].isUnused() || gcode[G].getInt() != 1 || gcode.movesteps == 0)
    return;

  if(nextg[G].isUnused() || nextg[G].getInt() != 1 || nextg.movesteps == 0)
  {
    computeAccel(gcode);
    return;
  }

  // Compute the maximum speed we can be going at the end o the move in order
  // to hit the next move at our best possible


  // Calculate the requested speed of each axis at its peak during this move,
  // including a direction sign.
  // Also calculate the biggest change in axis speeds over jerk.
  float axisspeeds[NUM_AXES];
  float nextspeeds[NUM_AXES];
  float mult1 = gcode.maxfeed / gcode.actualmm;
  float mult2 = nextg.maxfeed / nextg.actualmm;
  float biga = 0, bigb = 0;
  float jerka = gcode.maxfeed, jerkb = nextg.maxfeed;
  int whicha=0, whichb=0;
  bool fail = false;
  for(int ax=0;ax<NUM_AXES;ax++)
  {
    if(gcode.axisdirs[ax] != nextg.axisdirs[ax] && gcode.axismovesteps[ax] != 0 && nextg.axismovesteps[ax] != 0)
    {
      fail = true;
#ifdef DEBUG_OPT
      HOST.labelnum("dirchange ax:", ax);
#endif      
    }

    axisspeeds[ax] = (float)((float)gcode.axismovesteps[ax] / AXES[ax].getStepsPerMM()) * mult1;
    nextspeeds[ax] = (float)((float)nextg.axismovesteps[ax] / AXES[ax].getStepsPerMM()) * mult2;

    if(axisspeeds[ax] > biga)
    {
      biga = axisspeeds[ax];
      whicha = ax;
    }

    if(nextspeeds[ax] > bigb)
    {
      bigb = nextspeeds[ax];
      whichb = ax;
    }

    if(nextg.axismovesteps[ax] != 0 and AXES[ax].getStartFeed() < jerka)
      jerka = AXES[ax].getStartFeed();

    if(nextg.axismovesteps[ax] != 0 and AXES[ax].getStartFeed() < jerkb)
      jerkb = AXES[ax].getStartFeed();
  }
  if(!fail)
  {
    float smalla = biga, smallb = bigb;
    if(gcode.axisdirs[ax] != nextg.axisdirs[ax])
    {
      if(gcode.axismovesteps[ax] == 0)
        smalla = 0;
      if(nextg.axismovesteps[ax] == 0)
        smallb = 0;
    }

    for(int ax=0;ax<NUM_AXES;ax++)
    {
      if(axisspeeds[ax] > 0 && axisspeeds[ax] < smalla)
        smalla = axisspeeds[ax];
      if(nextspeeds[ax] > 0 && nextspeeds[ax] < smallb)
        smallb = nextspeeds[ax];
    }

    float rata=1, ratb=1;
    if(smalla != 0 && biga - smalla > jerka)
    {
      rata = jerka / (biga - smalla);
    }
    if(smallb != 0 && bigb - smallb > jerkb)
    {
      ratb = jerkb / (bigb - smallb);
    }

#ifdef DEBUG_OPT
    HOST.labelnum("biga:",biga,false);
    HOST.labelnum(" rata:",rata,false);
    HOST.labelnum(" smalla:",smalla,false);
    HOST.labelnum(" jerka:",jerka);

    HOST.labelnum("bigb:",bigb,false);
    HOST.labelnum(" ratb:",ratb,false);
    HOST.labelnum(" smallb:",smallb,false);
    HOST.labelnum(" jerkb:",jerkb);


#endif

    if(rata * biga > ratb * bigb)
    {
      rata = ratb;
      biga = bigb;
    }
    else
    {
      rata = ratb;
      biga = bigb;
    }

    gcode.endfeed   = min(gcode.maxfeed, rata * biga * gcode.axisratio[whicha]);
    nextg.startfeed = min(nextg.maxfeed, rata * biga * nextg.axisratio[whichb]);

    gcode.endfeed   = max(gcode.endfeed,   AXES[gcode.leading_axis].getStartFeed());
    nextg.startfeed = max(nextg.startfeed, AXES[nextg.leading_axis].getStartFeed());
  }
      
#ifdef DEBUG_OPT    
for(int ax=0;ax<NUM_AXES;ax++)
{
    HOST.labelnum("ASP[", ax, false);
    HOST.labelnum("]:", axisspeeds[ax],false);
    HOST.labelnum("->", (float)gcode.endfeed / gcode.axisratio[ax],false);
    HOST.labelnum(", ", (float)nextg.startfeed / nextg.axisratio[ax],false);
    HOST.labelnum("->", nextspeeds[ax]);
}
#endif
  // because we only lookahead one move, our maximum exit speed has to be either the desired
  // exit speed or the speed that the next move can reach to 0 (0+jerk, actually) during.

  // Speed at which we can reach 0 in the forseeable future
  float speedto0 = AXES[nextg.leading_axis].
                    getSpeedAtEnd(AXES[nextg.leading_axis].getStartFeed(),
                                  nextg.accel, 
                                  nextg.movesteps);

  if(speedto0 < nextg.startfeed)
    nextg.startfeed = speedto0;
    
  // now speedto0 is the maximum speed the primary in the next move can be going.  We need the equivalent speed of the primary in this move.
  speedto0 = (speedto0 * gcode.axisratio[nextg.leading_axis]);
  if(speedto0 < gcode.endfeed)
  {
#ifdef DEBUG_OPT
    HOST.labelnum("st0:",gcode.endfeed,false);
    HOST.labelnum(",",speedto0);
#endif
    gcode.endfeed = speedto0;
    nextg.startfeed = gcode.endfeed / gcode.axisratio[nextg.leading_axis];
  }

  computeAccel(gcode, &nextg);

#endif // LOOKAHEAD
}


void Motion::computeAccel(GCode& gcode, GCode* nextg)
{
  if(gcode[G].isUnused() || gcode[G].getInt() != 1)
    return;

  if(gcode.movesteps == 0)
    return;

  // Two cases:
  // 1) start speed < end speed; we get to accelerate for free up to end.
  // 2) start speed >= end speed; we must plan decel irst, then add in appropriate accel.
  if(gcode.startfeed < gcode.endfeed)
  {
    uint32_t distance_to_end = AXES[gcode.leading_axis].getAccelDist(gcode.startfeed, gcode.endfeed, gcode.accel);
    uint32_t distance_to_max = AXES[gcode.leading_axis].getAccelDist(gcode.startfeed, gcode.maxfeed, gcode.accel);
    // start is less than end, and we cannot accelerate enough to make the end in time.
    if(distance_to_end >= gcode.movesteps)
    {
      gcode.decel_from = 0;
      gcode.accel_until = 0;
      // TODO: fix - should be speed we will reach + jerk
      gcode.endfeed = AXES[gcode.leading_axis].getSpeedAtEnd(gcode.startfeed, gcode.accel, gcode.movesteps);
      if(nextg != NULL)
        nextg->startfeed = gcode.endfeed / nextg->axisratio[gcode.leading_axis];
    }
    else // start is less than end, and we have extra room to accelerate.
    {
      uint32_t halfspace = (gcode.movesteps - distance_to_end)/2;
      distance_to_max -= distance_to_end;
      if(distance_to_max <= halfspace)
      {
        // Plateau
        gcode.accel_until = gcode.movesteps - distance_to_end - distance_to_max;
        gcode.decel_from  = distance_to_max;
      }
      else
      {
        // Peak
        gcode.accel_until = gcode.movesteps - distance_to_end - halfspace;
        gcode.decel_from  = halfspace;
      }
    }
  }
  else // start speed >= end speed... must decelrate primarily, accel if time.
  {
    uint32_t distance_to_end = AXES[gcode.leading_axis].getAccelDist(gcode.endfeed, gcode.startfeed, gcode.accel);
    uint32_t distance_to_max = AXES[gcode.leading_axis].getAccelDist(gcode.startfeed, gcode.maxfeed, gcode.accel);
    if(distance_to_end >= gcode.movesteps)
    {
      // TODO: this is NOT GOOD.  Throw an error.
#ifdef DEBUG_ACCEL
      HOST.labelnum("TOO FAST! ", gcode.movesteps, false);
      HOST.labelnum(", needs ", distance_to_end, false);
      HOST.labelnum(" at ", gcode.accel, false);
      HOST.labelnum(" - ", gcode.startfeed, false);
      HOST.labelnum(" - ", gcode.endfeed);
#endif      
      gcode.decel_from = gcode.movesteps;
      gcode.accel_until = gcode.movesteps;
      // TODO: fix - should be speed we will reach + jerk
      gcode.endfeed = AXES[gcode.leading_axis].getSpeedAtEnd(gcode.startfeed, -gcode.accel, gcode.movesteps);
      if(nextg != NULL)
        nextg->startfeed = max(gcode.endfeed * nextg->axisratio[gcode.leading_axis], nextg->startfeed);
    }
    else  // lots of room to decelerate
    {
      uint32_t halfspace = (gcode.movesteps - distance_to_end)/2;
      if(distance_to_max <= halfspace)
      {
        // Plateau
        gcode.accel_until = gcode.movesteps - distance_to_max;
        gcode.decel_from  = distance_to_max + distance_to_end;
      }
      else
      {
        // Peak
        gcode.accel_until = gcode.movesteps - halfspace;
        gcode.decel_from  = halfspace + distance_to_end;
      }
    }
  }

  gcode.currentinterval = AXES[gcode.leading_axis].int_interval_from_feedrate(gcode.startfeed);
  gcode.currentfeed = gcode.startfeed;

  if(nextg != NULL && nextg->startfeed > nextg->maxfeed)
    nextg->startfeed = nextg->maxfeed;

#ifdef DEBUG_ACCEL    
  HOST.labelnum("lines:",gcode.linenum,false);
  if(nextg != NULL)
    HOST.labelnum("-",nextg->linenum,false);
  HOST.labelnum(" steps:", gcode.movesteps,false);
  HOST.labelnum(" au1:", gcode.accel_until,false);
  HOST.labelnum(" df1:", gcode.decel_from,false);
  HOST.labelnum(" attain: ", AXES[gcode.leading_axis].getSpeedAtEnd(gcode.startfeed, gcode.accel, gcode.movesteps-gcode.accel_until), false);
  HOST.labelnum(" start: ", gcode.startfeed, false);
  HOST.labelnum(" max: ", gcode.maxfeed, false);
  HOST.labelnum(" end: ", gcode.endfeed, false);
  if(nextg != NULL)
  {
    HOST.labelnum(" nextstart: ", nextg->startfeed, false);
    HOST.labelnum(" nextmax: ", nextg->maxfeed,false);
    HOST.labelnum(" nextsteps:", nextg->movesteps,false);
    HOST.labelnum(" au2:", nextg->accel_until,false);
    HOST.labelnum(" df2:", nextg->decel_from);
  }
  else
  {
    HOST.write(" - unopt.\n");
  }
#endif
}

void Motion::gcode_execute(GCode& gcode)
{
  // Only execute codes that are prepared.
  if(gcode.state < GCode::PREPARED)
    return; // TODO: This should never happen now and is an error.
  // Don't execute codes that are ACTIVE or DONE (ACTIVE get handled by interrupt)
  if(gcode.state > GCode::PREPARED)
    return;

  // Make sure they have configured the axis!
  if(AXES[0].isInvalid())
  {
    Host::Instance(gcode.source).write_P(PSTR("!! AXIS ARE NOT CONFIGURED !!\n"));
    gcode.state = GCode::DONE;
    return;
  }

  if(gcode.movesteps == 0)
  {
    gcode.state = GCode::DONE;
    return;
  }

  // set axis move data, invalidate all precomputes if bad data
  for(int ax=0;ax<NUM_AXES;ax++)
  {
    if(!AXES[ax].setupMove(gcode.startpos[ax], gcode.axisdirs[ax], gcode.axismovesteps[ax]))
    {
      GCODES.Invalidate();
      return;
    }
    deltas[ax] = gcode.axismovesteps[ax];
    errors[ax] = gcode.movesteps >> 1;
  }

  // setup pointer to current move data for interrupt
  gcode.state = GCode::ACTIVE;
  current_gcode = &gcode;

  setInterruptCycles(gcode.currentinterval);
  enableInterrupt();
}

bool Motion::axesAreMoving() 
{ 
  for(int ax=0;ax<NUM_AXES;ax++) 
    if(AXES[ax].isMoving()) return true; 
  
  return false; 
}


void Motion::writePositionToHost(GCode& gc)
{
  for(int ax=0;ax<NUM_AXES;ax++)
  {
    Host::Instance(gc.source).write(ax > Z ? 'A' - Z - 1 + ax : 'X' + ax); 
    Host::Instance(gc.source).write(':'); 
    Host::Instance(gc.source).write(AXES[ax].getCurrentPosition(),0,4); 
    Host::Instance(gc.source).write(' ');
  }
}



void Motion::handleInterrupt()
{
  if(busy)
    return;

#ifdef INTERRUPT_STEPS
  busy = true;
  //resetTimer();
  disableInterrupt();
  sei();
#endif  

  // interruptOverflow for step intervals > 16bit
  if(interruptOverflow > 0)
  {
    interruptOverflow--;
    enableInterrupt();
    busy=false;
    return;
  }

  if(current_gcode->movesteps == 0)
  {
    disableInterrupt();
    current_gcode->state = GCode::DONE;
    busy=false;
    return;
  }

  current_gcode->movesteps--;

  // Bresenham-style axis alignment algorithm
  for(ax=0;ax<NUM_AXES;ax++)
  {
    if(ax == current_gcode->leading_axis)
    {
      AXES[ax].doStep();
      continue;
    }

    errors[ax] = errors[ax] - deltas[ax];
    if(errors[ax] < 0)
    {
      AXES[ax].doStep();
      errors[ax] = errors[ax] + deltas[current_gcode->leading_axis];
    }
  }


  accelsteps = 0;
  current_gcode->accel_timer += current_gcode->currentinterval;
  // Handle acceleration and deceleration
  while(current_gcode->accel_timer > ACCEL_INC_TIME)
  {
    current_gcode->accel_timer -= ACCEL_INC_TIME;
    accelsteps++;
  }

  if(accelsteps)
  {
    if(current_gcode->movesteps >= current_gcode->accel_until && current_gcode->currentfeed < current_gcode->maxfeed)
    { 
      current_gcode->currentfeed += current_gcode->accel_inc * accelsteps;

      if(current_gcode->currentfeed > current_gcode->maxfeed)
        current_gcode->currentfeed = current_gcode->maxfeed;

      current_gcode->currentinterval = AXES[current_gcode->leading_axis].int_interval_from_feedrate(current_gcode->currentfeed);

      setInterruptCycles(current_gcode->currentinterval);
    }
    else if(current_gcode->movesteps <= current_gcode->decel_from && current_gcode->currentfeed > current_gcode->endfeed)
    { 
      current_gcode->currentfeed -= current_gcode->accel_inc * accelsteps;

      if(current_gcode->currentfeed < current_gcode->endfeed)
        current_gcode->currentfeed = current_gcode->endfeed;

      current_gcode->currentinterval = AXES[current_gcode->leading_axis].int_interval_from_feedrate(current_gcode->currentfeed);

      setInterruptCycles(current_gcode->currentinterval);
    }

  }


  if(!axesAreMoving())
  {
    current_gcode->movesteps = 0;
  }
      
  if(current_gcode->movesteps == 0)
  {
    disableInterrupt();
    current_gcode->state = GCode::DONE;
  }
#ifdef INTERRUPT_STEPS
  else
    enableInterrupt();

  busy = false;
#endif  
}

void Motion::setupInterrupt() 
{
  // 16-bit registers that must be set/read with interrupts disabled:
  // TCNTn, OCRnA/B/C, ICRn
  // "CTC" and no prescaler.
  //TCCR1A = 0;
  //TCCR1B = _BV(WGM12) | _BV(CS10);
  // "Fast PWM", top = OCR1A
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    TCCR1A = _BV(WGM11) | _BV(WGM10);
    TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);
    // Clear flag
    TIFR1 &= ~(_BV(OCF1A));
    // Enable timer
    PRR0 &= ~(_BV(PRTIM1));
    // Outcompare Compare match A interrupt
    TIMSK1 &= ~(_BV(OCIE1A));
    // Clear flag
    TIFR1 &= ~(_BV(OCF1A));
  }
}

void Motion::resetTimer()
{
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    // Disabel timer
    PRR0 |= _BV(PRTIM1);
    // unSET flag
    TIFR1 &= ~(_BV(OCF1A));
    // Reset Timer
    TCNT1=0;
  }
}


void Motion::enableInterrupt() 
{
  // Outcompare Compare match A interrupt
  TIMSK1 |= _BV(OCIE1A);
}

void Motion::disableInterrupt() 
{
  // Outcompare Compare match A interrupt
  TIMSK1 &= ~(_BV(OCIE1A));
}

void Motion::setInterruptCycles(unsigned long cycles) 
{
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    if(cycles > 60000)
    {
      OCR1A = 60000;
      interruptOverflow = cycles / 60000;
    }
    else if(cycles < MIN_INTERVAL)
      OCR1A = MIN_INTERVAL;
    else
      OCR1A = cycles;
  }
}




ISR(TIMER1_COMPA_vect)
{
  MOTION.handleInterrupt();
}
