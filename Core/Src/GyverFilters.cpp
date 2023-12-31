#include "GyverFilters.h"

#define MEDIAN_STOPPER 0  


//***************** return Max and Min func ***********************

int max(int a, int b)
{
	if(a > b)
	{
		return a;
	}
	else
	{
		return b;
	}
}

int min(int a, int b)
{
	if(a < b)
	{
		return a;
	}
	else
	{
		return b;
	}
}
// ***************************** GFilterRA *****************************
GFilterRA::GFilterRA() {}

GFilterRA::GFilterRA(float coef, uint16_t interval) {
	_coef = coef;
	_filterInterval = interval;
}

GFilterRA::GFilterRA(float coef) {
	_coef = coef;
}

void GFilterRA::setCoef(float coef) {
	_coef = coef;
}
void GFilterRA::setStep(uint16_t interval) {
	_filterInterval = interval;
}

float GFilterRA::filteredTime(int32_t value) {
//	if (HAL_GetTick() - _filterTimer >= _filterInterval) {
//		_filterTimer = HAL_GetTick();
		return GFilterRA::filtered(value);

}

//float GFilterRA::filteredTime(float value) {
//	if (HAL_GetTick() - _filterTimer >= _filterInterval) {
//		_filterTimer = HAL_GetTick();
//		return GFilterRA::filtered(value);
//	}
//}

float GFilterRA::filtered(int32_t value) {
	_lastValue += ((float)value - _lastValue) * _coef;
	return _lastValue;
}

float GFilterRA::filtered(float value) {
	_lastValue += (float)(value - _lastValue) * _coef;
	return _lastValue;
}

//Адаптивные фильтры. _coef можно менять на другие значения
float GFilterRA::filteredAdapt(int32_t value, int diff)
{
	//Adapt. Evolve. Survive
	if (abs(value - _lastValue) > diff)
		_coef = 0.95;
	else _coef = 0.05;

	_lastValue += (float)(value - _lastValue) * _coef;
	return _lastValue;
	//Adapt. Evolve. Survive
}

float GFilterRA::filteredAdapt(float value, float diff)
{
	//Adapt. Evolve. Survive
	if (abs(value - _lastValue) > diff)
		_coef = 0.95;
	else _coef = 0.05;

	_lastValue += (float)(value - _lastValue) * _coef;
	return _lastValue;
	//Adapt. Evolve. Survive
}

// ***************************** GMedian3 *****************************
GMedian3::GMedian3() {}

int GMedian3::filtered(long value) {
	buffer[counter] = value;
	if (++counter > 2) counter = 0;
	
	int a = buffer[0];
	int b = buffer[1];
	int c = buffer[2];
	int middle;
	
	if ((a <= b) && (a <= c)) {
		middle = (b <= c) ? b : c;
	}
	else {
		if ((b <= a) && (b <= c)) {
			middle = (a <= c) ? a : c;
		}
		else {
			middle = (a <= b) ? a : b;
		}
	}
	return middle;
}
// ***************************** GMedian *****************************
GMedian::GMedian() {}
uint32_t GMedian::filtered(uint32_t value) {

//	  static int bufMedian[3];
//	  static int countMedian = 0;
//	  bufMedian[countMedian] = value;
//	  if (++countMedian >= 3) countMedian = 0;
//	  return (max(bufMedian[0], bufMedian[1]) == max(bufMedian[1], bufMedian[2])) ? max(bufMedian[0], bufMedian[2]) : max(bufMedian[1], min(bufMedian[0], bufMedian[2]));

	struct pair
	{
		struct pair   *point;                              /* Pointers forming list linked in sorted order */
		uint32_t  value;                                   /* Values to sort */
	};
	static struct pair buffer[MEDIAN_FILTER_SIZE] = {0}; /* Buffer of nwidth pairs */
	static struct pair *datpoint = buffer;               /* Pointer into circular buffer of data */
	static struct pair small = {NULL, MEDIAN_STOPPER};          /* Chain MEDIAN_STOPPER */
	static struct pair big = {&small, 0};                /* Pointer to head (largest) of linked list.*/

	struct pair *successor;                              /* Pointer to successor of replaced data item */
	struct pair *scan;                                   /* Pointer used to scan down the sorted list */
	struct pair *scanold;                                /* Previous value of scan */
	struct pair *median;                                 /* Pointer to median */
	uint32_t i;

	if (value == MEDIAN_STOPPER)
	{
		value = MEDIAN_STOPPER + 1;                             /* No MEDIAN_STOPPERs allowed. */
	}

	if ( (++datpoint - buffer) >= MEDIAN_FILTER_SIZE)
	{
		datpoint = buffer;                               /* Increment and wrap data in pointer.*/
	}

	datpoint->value = value;                           /* Copy in new value */
	successor = datpoint->point;                       /* Save pointer to old value's successor */
	median = &big;                                     /* Median initially to first in chain */
	scanold = NULL;                                    /* Scanold initially null. */
	scan = &big;                                       /* Points to pointer to first (largest) value in chain */

	/* Handle chain-out of first item in chain as special case */
	if (scan->point == datpoint)
	{
		scan->point = successor;
	}
	scanold = scan;                                     /* Save this pointer and   */
	scan = scan->point ;                                /* step down chain */

	/* Loop through the chain, normal loop exit via break. */
	for (i = 0 ; i < MEDIAN_FILTER_SIZE; ++i)
	{
		/* Handle odd-numbered item in chain  */
		if (scan->point == datpoint)
		{
			scan->point = successor;                      /* Chain out the old value.*/
		}

		if (scan->value < value)                        /* If value is larger than scanned value,*/
		{
			datpoint->point = scanold->point;             /* Chain it in here.  */
			scanold->point = datpoint;                    /* Mark it chained in. */
			value = MEDIAN_STOPPER;
		};

		/* Step median pointer down chain after doing odd-numbered element */
		median = median->point;                       /* Step median pointer.  */
		if (scan == &small)
		{
			break;                                      /* Break at end of chain  */
		}
		scanold = scan;                               /* Save this pointer and   */
		scan = scan->point;                           /* step down chain */

		/* Handle even-numbered item in chain.  */
		if (scan->point == datpoint)
		{
			scan->point = successor;
		}

		if (scan->value < value)
		{
			datpoint->point = scanold->point;
			scanold->point = datpoint;
			value = MEDIAN_STOPPER;
		}

		if (scan == &small)
		{
			break;
		}

		scanold = scan;
		scan = scan->point;
	}
	return median->value;
}

// ***************************** GABfilter *****************************
GABfilter::GABfilter(float delta, float sigma_process, float sigma_noise) {
	setParameters(delta, sigma_process, sigma_noise);
}
void GABfilter::setParameters(float delta, float sigma_process, float sigma_noise) {
	dt = delta;
	float lambda = (float)sigma_process * dt * dt / sigma_noise;
	float r = (4 + lambda - (float)sqrt(8 * lambda + lambda * lambda)) / 4;
	a = (float)1 - r * r;
	b = (float)2 * (2 - a) - 4 * (float)sqrt(1 - a);
}
float GABfilter::filtered(float value) {
	xm = value;
	xk = xk_1 + ((float) vk_1 * dt );
	vk = vk_1;
	rk = xm - xk;
	xk += (float)a * rk;
	vk += (float)( b * rk ) / dt;
	xk_1 = xk;
	vk_1 = vk;
	return xk_1;
}

// ***************************** GKalman *****************************

GKalman::GKalman(float mea_e, float est_e, float q) {
	GKalman::setParameters(mea_e, est_e, q);
}
GKalman::GKalman(float mea_e, float q) {
	GKalman::setParameters(mea_e, mea_e, q);
}

void GKalman::setParameters(float mea_e, float est_e, float q) {
	_err_measure = mea_e;
	_err_estimate = est_e;
	_q = q;
}
void GKalman::setParameters(float mea_e, float q) {
	GKalman::setParameters(mea_e, mea_e, q);
}

float GKalman::filtered(float value) {
	float _kalman_gain, _current_estimate;
	_kalman_gain = _err_estimate / (_err_estimate + _err_measure);
	_current_estimate = _last_estimate + _kalman_gain * (value - _last_estimate);
	_err_estimate =  (1.0 - _kalman_gain)*_err_estimate + fabs(_last_estimate-_current_estimate)*_q;
	_last_estimate=_current_estimate;

	return _current_estimate;
}


//
//GKalman::GKalman(float mea_e, float est_e, float q) {
//	GKalman::setParameters(mea_e, est_e, q);
//}
//GKalman::GKalman(float mea_e, float q) {
//	GKalman::setParameters(mea_e, mea_e, q);
//}
//
//void GKalman::setParameters(float mea_e, float est_e, float q) {
//	_err_measure = mea_e;
//	_err_estimate = est_e;
//	_q = q;
//}
//void GKalman::setParameters(float mea_e, float q) {
//	GKalman::setParameters(mea_e, mea_e, q);
//}
//
//float GKalman::filtered(float value) {
//	float _kalman_gain, _current_estimate;
//	_kalman_gain = _err_estimate / (_err_estimate + _err_measure);
//	_current_estimate = _last_estimate + _kalman_gain * (value - _last_estimate);
//	_err_estimate =  (1.0 - _kalman_gain)*_err_estimate + fabs(_last_estimate-_current_estimate)*_q;
//	_last_estimate=_current_estimate;
//
//	return _current_estimate;
//}

// ************************* GLinear ***************************
GLinear::GLinear() {}

void GLinear::compute(int *x_array, int *y_array, int arrSize) {
	int32_t sumX = 0, sumY = 0, sumX2 = 0, sumXY = 0;	
	arrSize /= sizeof(int);
	for (int i = 0; i < arrSize; i++) {		// для всех элементов массива
		sumX += x_array[i];
		sumY += (long)y_array[i];
		sumX2 += x_array[i] * x_array[i];
		sumXY += (long)y_array[i] * x_array[i];
	}
	a = (long)arrSize * sumXY;             // расчёт коэффициента наклона приямой
	a = a - (long)sumX * sumY;
	a = (float)a / (arrSize * sumX2 - sumX * sumX);
	b = (float)(sumY - (float)a * sumX) / arrSize;
	delta = a * arrSize;					// расчёт изменения
}

float GLinear::getA() {
	return a;
}

float GLinear::getB() {
	return b;
}

float GLinear::getDelta() {
	return delta;
}
