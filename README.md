# About

External for Windows version of Max 8 (Max/MSP). fl_funmod is a max external that transforms curve formatted lists to specified domains/range. This may not be that useful for amplitude envelopes but it is quite helpful for phasor driven objects that uses [curve~] instead of a fixed ramp.

- Input: 
	-curve (list)
	-domain and range (list: 3 float)
- Messages: 
	-"mode" 0 (lineal), 1 (curve)
	-"listdump" 0, 1
- Output:
	-curve (list)