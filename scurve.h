uint8_t cntF;
int prevacc;
int nextacc;
int curracc;
int scurve,has1, has2, has3, has4, has5, has6, has7;
int sg, ok;
int lsteps;


float s1, s2, s3, s4, s5, s6, s7;
float va,vi, vc, ve, vjerk1, vjerk7;
float ja, a1x, a1, a2, as3, as7, T, V, S, Sdest, tstep, tstepS;

#define jerk xyjerk


float preparejerk(float dis) {
  float v1, v2, v3, v4, v5, v6, v7;
  float t1, t2, t3, t4, t5, t6, t7;


  float delta1, delta7, saccelerate;
  float jerk6 = 0.16667 * jerk;
  //while (1)
  //{
  if (has1 || has3) {
    vjerk1 = fmin(vjerk1, fabs(vc - vi));
    if (has1 && has3)vjerk1 = vjerk1 * 0.5;
    t1 = sqrt(2.0 * vjerk1 / jerk); // calc time to reac vjerk segment 1
    delta1 = jerk6 * t1 * t1 * t1;
  }

  if (has1) { // if previous is not accelerate up, and vinitial less than vcruise
    v1 = vi + vjerk1;
    s1 = vi * t1 + delta1;
  } else { // doesnot have segment 1
    s1 = 0;
    v1 = vi;
  }

  // segment 2 constant acceleration until vcruise-vjerk
  // Vt = Vt1 + a.T
  // T = (Vt-Vt1)/a
  v2 = vc;
  if (has3)v2 = vc - vjerk1;
  if (v2 - v1 == 0)has2 = 0;
  if (has2) {
    t2 = (v2 - v1) / ja;
    s2 = v1 * t2 + 0.5 * ja * t2 * t2;
  } else {
    t2 = 0;
    s2 = 0;
    // if no room for acceleration and jerk, then we must change the segment1 later
  }



  // segment 3 decelerate is same as segment 1
  if (has3)
  { //we have cruise followed by deceleration
    // check if dV is enough for jerk deceleration
    //if (vc-ve)>
    s3 = vc * t1 - delta1;
    t3 = t1;
  } else {
    s3 = 0;
    t3 = 0;
  }

  if (has1 || has3)as3 = t1 * jerk; else as3 = ja;

  v3 = vc;


  // ===========================================================================================
  // segment 5,6,7 is calculated inversed from ve to vc just like segment 1,2,3
  // segment 7, constant jerk until v increase vjerk

  if (has5 || has7) {
    vjerk7 = fmin(vjerk7, fabs(vc - ve));
    if (has5 && has7)vjerk7 = vjerk7 * 0.5;
    t7 = sqrt(2.0 * vjerk7 / jerk); // calc time to reac vjerk segment 1
    delta7 = jerk6 * t7 * t7 * t7;
  }

  if (has7) { // if previous is not accelerate up, and vinitial less than vcruise
    v6 = ve + vjerk7;
    s7 = ve * t7 + delta7;
  } else { // doesnot have segment 1
    s7 = 0;
    v6 = ve;
  }
  // segment 6 constant acceleration until vcruise-vjerk
  // Vt = Vt1 + a.T
  // T = (Vt-Vt1)/a

  if (has5)v5 = vc - vjerk7; else v5 = vc;
  if (v5 - v6 == 0)has6 = 0;
  if (has6) {
    t6 = (v5 - v6) / ja;
    s6 = v6 * t6 + 0.5 * ja * t6 * t6;
  } else {
    t6 = 0;
    s6 = 0;
    // if no room for acceleration and jerk, then we must change the segment1 later
  }

  // segment 5 decelerate is same as segment 7

  if (has5) { //if we have initial acceleration
    s5 = vc * t7 - delta7;
    t5 = t7;
  } else {
    s5 = 0;
    t5 = 0;
  }
  if (has5 || has7)as7 = t7 * jerk; else as7 = ja;


  // cruise must be calculate last, we must know how much distance from jerk ramp up and jerk ramp down
  // segment 4  = cruise
  saccelerate = s1 + s2 + s3 + s5 + s6 + s7;
  if (has4) {
    if (dis > saccelerate) { // we have cruise
      has4 = 1;
      s4 = dis - saccelerate;
    } else {
      s4 = 0;
      t4 = 0;
      has4 = 0;
    }
  } else s4 = 0;

  S = (s4 + saccelerate);
  tstep = TSTEP; // 0.0005s is good value //T/(Math.ceil(T)*1000);// 0.5ms //0.1/StepMM;
  V = vi;
  a1 = jerk * tstep * tstep;

  lsteps = 0;
  Sdest = 0;
  sg = 0;
  ok = 0;
  return S;
}


void prepareramp(int32_t bpos)
{

  tmove *m, *next;
  m = &move[bpos];
  scurve= (xyjerk>0);// && (m->dis>5);	
  // local m

  if (m->fs - m->fe >= m->delta) m->fn = m->fs;
  prevacc = curracc;
  if (bpos != (head)) {
    next = &move[nextbuff(bpos)];
    if (next->fs - next->fe >= next->delta) next->fn = next->fs;

    if ((next->fs == next->fn) && (next->fe < next->fn)) { // all down
      nextacc = -1;
    } else if ((next->fe == next->fn) && (next->fs < next->fn)) { // all up
      nextacc = 1;
    } else if ((next->fe == next->fn) && (next->fs == next->fn)) { // all up
      nextacc = 0;
    } else {
      nextacc = 1;
    }
  } else {
    //fe=0;
    nextacc = 0;
  }

  has4 = m->delta > fabs(m->fe - m->fs);

  if ((m->fs == m->fn) && (m->fe < m->fn)) { // all down
    curracc = -1;
  } else if ((m->fe == m->fn) && (m->fs < m->fn) && !has4) { // all up
    curracc = 1;
  } else if (m->fe == m->fn && m->fs == m->fn) { // all up
    curracc = 0;
  } else {
    if (m->fn > m->fe) curracc = -1; //else curracc = 0;
  }

  // lets check 1 by 1
  //
  ja = scurve?m->ac:m->ac*0.5;
  float mvjerk = scurve?(0.5 * ja * ja / jerk):0;

  vjerk1 = 0;
  // if
  // segment 1, prevacc <=0 and fs<fn
  has1 = scurve && (prevacc <= 0) && (m->fs < m->fn);
  // segment 3, curracc <=0 and fs<fn
  has3 = scurve && (has4) && (m->fs < m->fn);

  if (has1)vjerk1 += mvjerk;
  if (has3)vjerk1 += mvjerk;
  // segment 2, constant acceleration prevacc <=0 and fs<fn
  has2 = ((m->fn - m->fs) > vjerk1);

  vjerk7 = 0;
  //
  // segment 1, prevacc <=0 and fs<fn
  has7 = scurve && (nextacc >= 0) && (m->fe < m->fn);
  // segment 3, curracc <=0 and fs<fn
  has5 = scurve && (curracc <= 0 && has4) && (m->fe < m->fn);
  if (!has4 && (prevacc == -1) && (curracc == -1))has5 = 0;



  if (has7)vjerk7 += mvjerk;
  if (has5)vjerk7 += mvjerk;
  // segment 2, constant acceleration prevacc <=0 and fs<fn
  has6 = (m->fn - m->fe > vjerk7);



  m->status |= 4;
  vi = sqrt(m->fs);
  vc = sqrt(m->fn);
  ve = sqrt(m->fe);
  m->dis = preparejerk(m->dis);
  tstepS = float(totalstep) / m->dis;
  S = 0;
  CORELOOP
}

void machinemove(float dis, float vel) {
  // current position in motor stepper steps
  // tstepS is a constant to get motor stepper steps from distance (mm)
  int steps = (dis * tstepS);
  int ds = steps - lsteps;
  if (ds > 0) { // >0 mean motor stepper need to move
	  //if (sg < 3)zprintf(PSTR("%dxV%f "), fi(ds),ff(vel));
	  //else zprintf(PSTR("."));
    lsteps = steps; // save last step position
    if (sg!=4 && ((++cntF)&4))
		xInvVel(dlp, vel); // T = CPU_CLOCK/Vel , only update every 4 step, increase if need faster motor
    for (int i = 0; i < ds; i++) {
      // do bresenham this step longs
      if (mctr>0) {
        cmd0 = 1; //step command
        // bresenham movement on all laxis and set motor motion bit to cmd0
        bresenham(0); //
        bresenham(1);
        bresenham(2);
        bresenham(3);

        // push T=CLOCK/V to timer command buffer
        cmd0 |= dlp << 5; // cmd0 is 32bit data contain all motor movement and the timing
        pushcmd();
      } else return;
      mctr--;
    }
    //zprintf(PSTR("<-%d\n"),fi(mctr));
    // send the t with other things needed to timerbufer
  } else if (ds < 0) {
    //zprintf(PSTR("Step back %d ??\n"),fi(ds));
  }
}

// curveloop is iterating each timestep (i set 0.001 sec) for changing velocity from constant jerk
// by change the initial a1x,a2 we can use this function for all 7 segment
// this mean the velocity only will change each 0.001 sec
// then in this timestep it check the lastS versus currentS in motor stepper steps
// it will move the motor (currentS - lastS) steps with current velocity
float fa;
int curveloop() {
  a2 += a1x; // jerk add to acceleration
  V += a2; // acceleration add to velocity
  //#define va V
  //if (V<MINCORNERSPEED)V=MINCORNERSPEED;
  va = (V < 0.5) ? 0.5 : V;
  va = ((va > vc) ? vc : va);
  S += va * tstep; // velocity add to Distance
  //S+=V*tstep; // velocity add to Distance
  
  
  machinemove(S, va); // run bresenham for machine for segment sg
  return S < Sdest; // continue until reach each segment distance , if reached, then need to initialize next segment
}

int coreloopscurve(){
  float lV = V;
  float lS = S;
	if (!ok) {
	  sg++;
	  // initialize each segment
	  if (sg == 1) { // constant jerk segment
		if (has1) {
		  a2 = 0;
		  a1x = a1;
		  S = 0;
		  Sdest = s1;
		} else sg++; // next segment
	  }
	  if (sg == 2) { // constant acc segment
		if (has2) {
		  a2 = as3 * tstep;
		  V = vi + (has1 ? vjerk1 : 0);
		  a1x = 0;
		  S = Sdest;
		  Sdest = Sdest + s2;
		} else sg++;
	  }
	  if (sg == 3) { // constant jerk deceleration segment
		if (has3) {
		  a2 = as3 * tstep;
		  V = vc - vjerk1;
		  a1x = -a1;
		  S = Sdest;
		  Sdest = Sdest + s3;
		} else sg++;
	  }
	  if (sg == 4) { // constant velocity segment
		if (has4) {
		  V = vc;
		  xInvVel(dlp, V);
		  a2 = 0;
		  a1x = 0;
		  S = Sdest;
		  Sdest = Sdest + s4;
		} else sg++;
	  }
	  if (sg == 5) {
		if (has5) {
		  a2 = 0;
		  a1x = -a1;
		  V = vc;
		  S = Sdest;
		  Sdest = Sdest + s5;
		} else sg++;
	  }
	  if (sg == 6) {
		if (has6) {
		  a2 = -as7 * tstep;
		  a1x = 0;
		  S = Sdest;
		  V = vc - (has5 ? vjerk7 : 0);
		  Sdest = Sdest + s6;
		} else sg++;
	  }
	  if (sg == 7) {
		if (has7) {
		  a1x = a1;
		  a2 = -as7 * tstep;
		  V = ve + vjerk7; //+a2*0.5; // needed to make sure we reach accurate exit velocity
		  S = Sdest;
		  Sdest = Sdest + s7;
		} else sg++;
	  }
	  if (sg == 8) {
		//zprintf(PSTR("End segment\n"));
		m = 0;
		return 0;
	  }
	}
	#ifdef output_enable
	if (sg==6){
	  zprintf(PSTR("\nSG:%d lS:%f lV:%f S:%f -> %f\n"), fi(sg), ff(lS), ff(lV), ff(S), ff(Sdest));
	  zprintf(PSTR("V:%f A:%f J:%f\n"), ff(V), ff(a2), ff(a1x));
	  zprintf(PSTR("T:%f TS:%f\n"), ff(tstep), ff(tstepS));
	}
	#endif
	ok = curveloop();
	return 1;
}
