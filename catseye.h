//---------------------------------------------------------
//	Cat's eye
//
//		©2016 Yuichiro Nakada
//---------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define CATS_RANDOM

#define CATS_SIGMOID
//#define CATS_TANH
//#define CATS_SCALEDTANH
//#define CATS_RELU
//#define CATS_ABS

//#define CATS_SIGMOID_CROSSENTROPY
#ifdef CATS_SIGMOID_CROSSENTROPY
// cross entropy df: (y - t) / (y * (1 - y))
// sigmoid function with cross entropy loss
#define CATS_SIGMOID
#define s_gain			1
#define ACT2(x)			ACT1(x)
#define DACT2(x)		DACT1(x)
// SoftmaxWithLoss
#else
// identify function with mse loss
#define s_gain			1
#define ACT2(x)			(x)
#define DACT2(x)		1
#endif

// activation function and derivative of activation function
#ifdef CATS_SIGMOID
// sigmoid function
#define ACT1(x)			(1.0 / (1.0 + exp(-x * s_gain)))
#define DACT1(x)		((1.0-x)*x * s_gain)	// ((1.0-sigmod(x))*sigmod(x))
#elif defined CATS_TANH
// tanh function
#define ACT1(x)			(tanh(x))
#define DACT1(x)		(1.0-x*x)		// (1.0-tanh(x)*tanh(x))
#elif defined CATS_SCALEDTANH
// scaled tanh function
#define ACT1(x)			(1.7159 * tanh(2.0/3.0 * x))
#define DACT1(x)		((2.0/3.0)/1.7159 * (1.7159-x)*(1.7159+x))
#elif defined CATS_RELU
// rectified linear unit function
#define ACT1(x)			(x>0 ? x : 0.0)
#define DACT1(x)		(x>0 ? 1.0 : 0.0)
#elif defined CATS_ABS
// abs function
#define ACT1(x)			(x / (1.0 + fabs(x)))
#define DACT1(x)		(1.0 / (1.0 + fabs(x))*(1.0 + fabs(x)))
#else
// identity function (output only)
#define ACT1(x)			(x)
#define DACT1(x)		(1.0)
#endif

#ifdef CATS_OPT_ADAGRAD
// AdaGrad (http://qiita.com/ak11/items/7f63a1198c345a138150)
#define eps 1e-8		// 1e-4 - 1e-8
#define OPT_CALC1(x)		this->e##x[i] += this->d[x-2][i]*this->d[x-2][i]
//#define OPT_CALC1(x)		this->e##x[i] += this->d[x-2][i]*this->d[x-2][i] *0.7
//#define OPT_CALC1(x)		this->e##x[i] = this->e##x[i]*(0.99+times/times*0.01) + this->d[x-2][i]*this->d[x-2][i]
//#define OPT_CALC2(n, x, y)	this->w[x-1][i*n+j] -= eta*this->o[x-1][i] *this->d[y-2][j] /sqrt(this->e##y[j])
#define OPT_CALC2(n, x, y)	this->w[x-1][i*n+j] -= eta*this->o[x-1][i] *this->d[y-2][j] /sqrt(this->e##y[j]+eps)

#elif defined CATS_OPT_ADAM
// Adam
#define eps 1e-8
#define beta1 0.9
#define beta2 0.999
#define OPT_CALC1(x)		this->m##x[i] = beta1*this->m##x[i] + (1.0-beta1) * this->d[x-2][i]; \
				this->v##x[i] = beta2*this->v##x[i] + (1.0-beta2) * this->d[x-2][i]*this->d[x-2][i]
#define OPT_CALC2(n, x, y)	this->w[x-1][i*n+j] -= eta*this->o[x-1][i] *this->m##y[j] /sqrt(this->v##y[j]+eps)

#elif defined CATS_OPT_RMSPROP
// RMSprop (http://cs231n.github.io/neural-networks-3/#anneal)
#define eps 1e-8		// 1e-4 - 1e-8
#define decay_rate 0.999	// [0.9, 0.99, 0.999]
#define OPT_CALC1(x)		this->e##x[i] = decay_rate * this->e##x[i] + (1.0-decay_rate)*this->d[x-2][i]*this->d[x-2][i]
#define OPT_CALC2(n, x, y)	this->w[x-1][i*n+j] -= eta*this->o[x-1][i] *this->d[y-2][j] /sqrt(this->e##y[j]+eps)

#elif defined CATS_OPT_RMSPROPGRAVES
// RMSpropGraves (https://github.com/pfnet/chainer/blob/master/chainer/optimizers/rmsprop_graves.py)
#define eps 1e-4
#define beta1 0.95
#define beta2 0.95
#define momentum 0.9
#define OPT_CALC1(x)		this->m##x[i] = beta1*this->m##x[i] + (1.0-beta1) * this->d[x-2][i]; \
				this->v##x[i] = beta2*this->v##x[i] + (1.0-beta2) * this->d[x-2][i]*this->d[x-2][i]
#define OPT_CALC2(n, x, y)	this->dl##x[j] = this->dl##x[j] * momentum - eta*this->o[x-1][i]*this->d[y-2][j] /sqrt(this->v##y[j] - this->m##y[j]*this->m##y[j]+eps); \
				this->w[x-1][i*n+j] += this->dl##x[j]

#elif defined CATS_OPT_MOMENTUM
// Momentum update (http://cs231n.github.io/neural-networks-3/#anneal)
#define momentum 0.9		// [0.5, 0.9, 0.95, 0.99]
#define OPT_CALC1(x)
#define OPT_CALC2(n, x, y)	this->dl##x[i] = momentum * this->dl##x[i] - eta*this->o[x-1][i] *this->d[y-2][j]; \
				this->w[x-1][i*n+j] += this->dl##x[i]

#else
// SGD (Vanilla update)
#define CATS_OPT_SGD
#define OPT_CALC1(x)
//#define OPT_CALC2(n, x, y)	this->w[x-1][i*n+j] -= eta*this->o[x-1][i] *this->d[y-2][j]
// SVM (http://d.hatena.ne.jp/echizen_tm/20110627/1309188711)
// ∂loss(w, x, t) / ∂w = ∂(λ - twx + α * w^2 / 2) / ∂w = - tx + αw
#define OPT_CALC2(n, x, y)	this->w[x-1][i*n+j] -= eta*this->o[x-1][i] *this->d[y-2][j] +this->w[x-1][i*n+j]*1e-8
#endif

void muladd(double *vec1, double *vec2, double a, int n)
{
	for (int i=0; i<n; i++) {
		vec1[i] += vec2[i] * a;
	}
}
double dot(double *vec1, double *vec2, int n)
{
	double s = 0.0;
	for (int i=0; i<n; i++) {
		s += vec1[i] * vec2[i];
	}
	return s;
}
double dotT(double *mat1, double *vec1, int r, int c)
{
	double s = 0.0;
	for (int i=0; i<r; i++) {
		s += mat1[i*c] * vec1[i];
	}
	return s;
}
/*void transpose(double *src, double *dst, int N, int M)
{
//	#pragma omp parallel for
	for (int i=0; i<M; i++) {
		for (int j=0; j<N; j++) {
			*dst++ = src[M*j + i];
		}
	}
}*/
int binomial(/*int n, */double p)
{
//	if (p<0 || p>1) return 0;
	int c = 0;
//	for (int i=0; i<n; i++) {
		double r = rand() / (RAND_MAX + 1.0);
		if (r < p) c++;
//	}
	return c;
}

typedef struct {
	// number of each layer
	int *layer, layers, *units;
	// input layer
//	double *x;
	// output layers [o = f(z)]
	double **z, **o;
	// error value
	double **d;
	// gradient value
	double *e2, *e3, *m2, *v2, *m3, *v3;
	double *dl1, *dl2;
	// weights
	double **w;
} CatsEye;

// identity function (output only)
double CatsEye_act_identity(double *x, int n, int len)
{
	return (x[n]);
}
double CatsEye_dact_identity(double *x, int n, int len)
{
	return (1.0);
}
// softmax function (output only)
double CatsEye_act_softmax(double *x, int n, int len)
{
	double alpha = x[0];
	for (int i=1; i<len; i++) if (alpha<x[i]) alpha = x[i];
	double numer = exp(x[n] - alpha);
	double denom = 0.0;
	for (int i=0; i<len; i++) denom += exp(x[i] - alpha);
	return (numer / denom);
}
double CatsEye_dact_softmax(double *x, int n, int len)
{
	return (x[n] * (1.0 - x[n]));
}
// sigmoid function
double CatsEye_act_sigmoid(double *x, int n, int len)
{
	return (1.0 / (1.0 + exp(-x[n] * s_gain)));
}
double CatsEye_dact_sigmoid(double *x, int n, int len)
{
	return ((1.0-x[n])*x[n] * s_gain);	// ((1.0-sigmod(x))*sigmod(x))
}
// tanh function
double CatsEye_act_tanh(double *x, int n, int len)
{
	return (tanh(x[n]));
}
double CatsEye_dact_tanh(double *x, int n, int len)
{
	return (1.0-x[n]*x[n]);		// (1.0-tanh(x)*tanh(x))
}

// activation function
double (*CatsEye_act[])(double *x, int n, int len) = {
	CatsEye_act_identity,
	CatsEye_act_softmax,
	CatsEye_act_sigmoid,
	CatsEye_act_tanh
};
double (*CatsEye_dact[])(double *x, int n, int len) = {
	CatsEye_dact_identity,
	CatsEye_dact_softmax,
	CatsEye_dact_sigmoid,
	CatsEye_dact_tanh
};

// caluculate forward propagation of input x
void CatsEye_layer_forward(double *x, double *w, double *z, double *o, int in, int out, int a)
{
	for (int i=0; i<out; i++) {
		z[i] = dotT(&w[i], x, in+1, out);
		o[i] = CatsEye_act[a](z, i, out);
	}
//	o[out] = 1;	// for bias
}
// caluculate back propagation
void CatsEye_layer_back(double *o, double *w, double *d, double *delta, int in, int out, int a)
{
	// calculate the error
	for (int i=0; i<in; i++) {
		d[i] = dot(&w[i*out], delta, out) * CatsEye_dact[a](o, i, in);
		//OPT_CALC1(2);
	}
	d[in] = dot(&w[in*out], delta, out) * CatsEye_dact[a](o, in, in);
}
void CatsEye_layer_update(double eta, double *o, double *w, double *d, int in, int out)
/*{
	// update the weights
	for (int i=0; i<in+1; i++) {
		for (int j=0; j<out; j++) {
			w[i*out+j] -= eta*o[i]*d[j];
			//OPT_CALC2(out, 1, 2);
		}
	}
}
void CatsEye_SVM_update(double eta, double *o, double *w, double *d, int in, int out)*/
{
	// update the weights
	for (int i=0; i<in+1; i++) {
		for (int j=0; j<out; j++) {
			// SVM (http://d.hatena.ne.jp/echizen_tm/20110627/1309188711)
			// ∂loss(w, x, t) / ∂w = ∂(λ - twx + α * w^2 / 2) / ∂w = - tx + αw
			w[i*out+j] -= eta*o[i]*d[j] + w[i*out+j]*1e-8;
		}
	}
}

/* constructor
 * n_in:  number of input layer
 * n_hid: number of hidden layer
 * n_out: number of output layer */
void CatsEye__construct(CatsEye *this, int n_in, int n_hid, int n_out, char *filename)
{
//	int n[] = { n_in, CATS_RANDOM, 0, n_hid, CATS_SIGMOID, CATS_MLP, n_out, CATS_SIGMOID, 0 };
	int n[] = { n_in, 0, 0, n_hid, 2, 0, n_out, 2, 0 };
	this->layers = 3;
	this->layer = malloc(sizeof(int)*this->layers*3);
	for (int i=0; i<this->layers*3; i++) this->layer[i] = n[i];
	this->units = malloc(sizeof(int)*this->layers);

	FILE *fp;
	if (filename) {
		fp = fopen(filename, "r");
		if (fp==NULL) return;
		fscanf(fp, "%d %d %d\n", &this->units[0], &this->units[1], &this->units[2]);
	} else {
		this->units[0] = n_in;
		this->units[1] = n_hid;
		this->units[2] = n_out;
	}

	// allocate inputs
	this->z = malloc(sizeof(double*)*(this->layers-1));
	this->z[0] = malloc(sizeof(double)*(this->units[1]+1));
	this->z[1] = malloc(sizeof(double)*this->units[2]);

	// allocate outputs
	this->o = malloc(sizeof(double*)*(this->layers));
	this->o[0] = malloc(sizeof(double)*(this->units[0]+1));
	this->o[1] = malloc(sizeof(double)*(this->units[1]+1));
	this->o[2] = malloc(sizeof(double)*this->units[2]);

	// allocate errors
	this->d = malloc(sizeof(double*)*(this->layers-1));
	this->d[0] = malloc(sizeof(double)*(this->units[1]+1));
	this->d[1] = malloc(sizeof(double)*this->units[2]);

	// allocate gradient
	this->e2 = calloc(1, sizeof(double)*(this->units[1]+1));
	this->e3 = calloc(1, sizeof(double)*this->units[2]);
	this->m2 = calloc(1, sizeof(double)*(this->units[1]+1));
	this->m3 = calloc(1, sizeof(double)*this->units[2]);
	this->v2 = calloc(1, sizeof(double)*(this->units[1]+1));
	this->v3 = calloc(1, sizeof(double)*this->units[2]);
	this->dl1 = malloc(sizeof(double)*(this->units[0]+1)*this->units[1]);
	this->dl2 = malloc(sizeof(double)*(this->units[1]+1)*this->units[2]);

	// allocate memories
	this->w = malloc(sizeof(double*)*(this->layers-1));
	this->w[0] = malloc(sizeof(double)*(this->units[0]+1)*this->units[1]);
	this->w[1] = malloc(sizeof(double)*(this->units[1]+1)*this->units[2]);

	if (filename) {
		for (int i=0; i<(this->units[0]+1)*this->units[1]; i++) {
			fscanf(fp, "%lf ", &this->w[0][i]);
		}
		for (int i=0; i<(this->units[1]+1)*this->units[2]; i++) {
			fscanf(fp, "%lf ", &this->w[1][i]);
		}
		fclose(fp);
	} else {
		// initialize weights (http://aidiary.hatenablog.com/entry/20150618/1434628272)
		// range depends on the research of Y. Bengio et al. (2010)
		srand((unsigned)(time(0)));
		double range = sqrt(6)/sqrt(this->units[0]+this->units[1]+2);
		srand((unsigned)(time(0)));
		for (int i=0; i<(this->units[0]+1)*this->units[1]; i++) {
			this->w[0][i] = 2.0*range*rand()/RAND_MAX-range;
		}
		for (int i=0; i<(this->units[1]+1)*this->units[2]; i++) {
			this->w[1][i] = 2.0*range*rand()/RAND_MAX-range;
		}
	}
}

// deconstructor
void CatsEye__destruct(CatsEye *this)
{
	// delete arrays
	free(this->z[0]);
	free(this->z[1]);
	free(this->z);
	free(this->o[0]);
	free(this->o[1]);
	free(this->o[2]);
	free(this->o);
	free(this->d[0]);
	free(this->d[1]);
	free(this->d);
	free(this->e2);
	free(this->e3);
	free(this->m2);
	free(this->m3);
	free(this->v2);
	free(this->v3);
	free(this->w[0]);
	free(this->w[1]);
	free(this->w);
	free(this->layer);
	free(this->units);
}

// caluculate forward propagation of input x
void CatsEye_forward(CatsEye *this, double *x)
{
	// calculation of input layer
	memcpy(this->o[0], x, this->units[0]*sizeof(double));
	this->o[0][this->units[0]] = 1;	// for bias
#ifdef CATS_DENOISING_AUTOENCODER
	// Denoising Autoencoder (http://kiyukuta.github.io/2013/08/20/hello_autoencoder.html)
	for (int i=0; i<this->units[0]; i++) {
		this->o[0][i] *= binomial(/*0.8(20%)*//*0.2*/0.5);
	}
#endif

	// caluculation of hidden layer [z = wx, o = f(z)]
	CatsEye_layer_forward(this->o[0], this->w[0], this->z[0], this->o[1], this->units[0], this->units[1], this->layer[1*3+1]);
	this->o[1][this->units[1]] = 1;	// for bias
/*	for (int j=0; j<this->units[1]; j++) {
		this->z[0][j] = 0;
		for (int i=0; i<this->units[0]+1; i++) {
			this->z[0][j] += this->w[0][i*this->units[1]+j]*this->o[0][i];
		}
		this->o[1][j] = ACT1(this->z[0][j]);
	}
	this->o[1][this->units[1]] = 1;	// for bias*/

	// caluculation of output layer
	CatsEye_layer_forward(this->o[1], this->w[1], this->z[1], this->o[2], this->units[1], this->units[2], this->layer[2*3+1]);
}

/* train: multi layer perceptron
 * x: train data (number of elements is in*N)
 * t: correct label (number of elements is N)
 * N: data size
 * repeat: repeat times
 * eta: learning rate (1e-6 to 1) */
void CatsEye_train(CatsEye *this, double *x, void *t, int N, int repeat, double eta)
{
	for (int times=0; times<repeat; times++) {
		double err = 0;
#ifndef CATS_OPT_SGD
		memset(this->e3, 0, sizeof(double)*this->units[2]);
		memset(this->e2, 0, sizeof(double)*this->units[1]);
#endif
#ifndef CATS_RANDOM
		for (int sample=0; sample<N; sample++) {
#else
		for (int n=0; n<1500/*N*/; n++) {
			int sample = (rand()/(RAND_MAX+1.0)) * N;	// 0 <= rand < 1
#endif
			// forward propagation
			CatsEye_forward(this, x+sample*this->units[0]);

			// calculate the error of output layer
			for (int i=0; i<this->units[2]; i++) {
#ifndef CATS_LOSS_MSE
				// http://d.hatena.ne.jp/echizen_tm/20110606/1307378609
				// E = max(0, -twx), ∂E / ∂w = max(0, -tx)
				if (((int*)t)[sample] == i) {	// 1-of-K
					this->d[1][i] = this->o[2][i]-1;
				} else {
					this->d[1][i] = this->o[2][i];
				}
#else
				// http://qiita.com/Ugo-Nama/items/04814a13c9ea84978a4c
				// https://github.com/nyanp/tiny-cnn/wiki/%E5%AE%9F%E8%A3%85%E3%83%8E%E3%83%BC%E3%83%88
				this->d[1][i] = this->o[2][i]-((double*)t)[sample*this->units[2]+i];
//				this->d[1][i] = (this->o[2][i]-((double*)t)[sample*this->units[2]+i]) * DACT2(this->o[2][i]);
#endif
//				this->e3[i] += this->d[1][i]*this->d[1][i];
				OPT_CALC1(3);
			}
			// calculate the error of hidden layer
			CatsEye_layer_back(this->o[1], this->w[1], this->d[0], this->d[1], this->units[1], this->units[2], this->layer[1*3+1]);
#if 0
			for (int i=0; i<this->units[1]; i++) {
/*				double tmp = 0;
				for (int l=0; l<this->units[2]; l++) {
					tmp += this->w[1][j*this->units[2]+l]*this->d[1][l];
				}*/
				//this->d[0][j] = tmp * DACT1(this->z[0][j]);
				this->d[0][i] = dot(&this->w[1][i*this->units[2]], this->d[1], this->units[2]) * DACT1(this->o[1][i]);
				OPT_CALC1(2);
			}
			this->d[0][this->units[1]] = dot(&this->w[1][this->units[1]*this->units[2]], this->d[1], this->units[2]) * DACT1(this->o[1][this->units[1]]);
#endif
			// update the weights of hidden layer
			CatsEye_layer_update(eta, this->o[0], this->w[0], this->d[0], this->units[0], this->units[1]);
/*			for (int i=0; i<this->units[0]+1; i++) {
				for (int j=0; j<this->units[1]; j++) {
					OPT_CALC2(this->units[1], 1, 2);
				}
				//muladd(&this->w[0][i*this->units[1]], this->d[0], -eta*this->o[0][i], this->units[1]);
			}*/
			// update the weights of output layer
			CatsEye_layer_update(eta, this->o[1], this->w[1], this->d[1], this->units[1], this->units[2]);
#ifdef CATS_AUTOENCODER
			// tied weight
			double *dst = this->w[1];
			for (int i=0; i<this->units[1]; i++) {
				for (int j=0; j<this->units[0]; j++) {
					this->w[1][j + this->units[1]*i] = this->w[0][this->units[1]*j + i];
				}
			}
#endif

			// calculate the mean squared error
			double mse = 0;
			for (int i=0; i<this->units[2]; i++) {
				mse += 0.5 * (this->d[1][i] * this->d[1][i]);
			}
			err = 0.5 * (err + mse);

/*			if (isnan(this->d[1][0])) {
				printf("epochs %d, samples %d, mse %f\n", times, sample, err);
				break;
			}*/
		}
		printf("epochs %d, mse %f\n", times, err);
	}
}

// return most probable label to the input x
int CatsEye_predict(CatsEye *this, double *x)
{
	// forward propagation
	CatsEye_forward(this, x);
	// biggest output means most probable label
	double max = this->o[2][0];
	int ans = 0;
	for (int i=1; i<this->units[2]; i++) {
		if (this->o[2][i] > max) {
			max = this->o[2][i];
			ans = i;
		}
	}
	return ans;
}

// save weights to csv file
int CatsEye_save(CatsEye *this, char *filename)
{
	FILE *fp = fopen(filename, "w");
	if (fp==NULL) return -1;

	fprintf(fp, "%d %d %d\n", this->units[0], this->units[1], this->units[2]);

	int i;
	for (i=0; i<(this->units[0]+1)*this->units[1]-1; i++) {
		fprintf(fp, "%lf ", this->w[0][i]);
	}
	fprintf(fp, "%lf\n", this->w[0][i]);

	for (i=0; i<(this->units[1]+1)*this->units[2]-1; i++) {
		fprintf(fp, "%lf ", this->w[1][i]);
	}
	fprintf(fp, "%lf\n", this->w[1][i]);

	fclose(fp);
	return 0;
}

// save weights to json file
int CatsEye_saveJson(CatsEye *this, char *filename)
{
	FILE *fp = fopen(filename, "w");
	if (fp==NULL) return -1;

	fprintf(fp, "var config = [%d,%d,%d];\n", this->units[0], this->units[1], this->units[2]);

	int i;
	fprintf(fp, "var w1 = [");
	for (i=0; i<(this->units[0]+1)*this->units[1]-1; i++) {
		fprintf(fp, "%lf,", this->w[0][i]);
	}
	fprintf(fp, "%lf];\n", this->w[0][i]);

	fprintf(fp, "var w2 = [");
	for (i=0; i<(this->units[1]+1)*this->units[2]-1; i++) {
		fprintf(fp, "%lf,", this->w[1][i]);
	}
	fprintf(fp, "%lf];\n", this->w[1][i]);

	fclose(fp);
	return 0;
}

// save weights to binary file
int CatsEye_saveBin(CatsEye *this, char *filename)
{
	FILE *fp = fopen(filename, "wb");
	if (fp==NULL) return -1;

	fwrite(&this->units[0], sizeof(this->units[0]), 1, fp);
	fwrite(&this->units[1], sizeof(this->units[1]), 1, fp);
	fwrite(&this->units[2], sizeof(this->units[2]), 1, fp);

	//fwrite(this->w[0], sizeof(double)*(this->units[0]+1)*this->units[1], 1, fp);
	//fwrite(this->w[1], sizeof(double)*(this->units[1]+1)*this->units[2], 1, fp);
	for (int i=0; i<(this->units[0]+1)*this->units[1]; i++) {
		float a = this->w[0][i];
		fwrite(&a, sizeof(float), 1, fp);
	}
	for (int i=0; i<(this->units[1]+1)*this->units[2]; i++) {
		float a = this->w[1][i];
		fwrite(&a, sizeof(float), 1, fp);
	}

	fclose(fp);
	return 0;
}

// w1
void CatsEye_visualizeWeights(CatsEye *this, int n, int size, unsigned char *p, int width)
{
	double *w = &this->w[0][n];
	double max = w[0];
	double min = w[0];
	for (int i=1; i<this->units[0]; i++) {
		if (max < w[i * this->units[1]]) max = w[i * this->units[1]];
		if (min > w[i * this->units[1]]) min = w[i * this->units[1]];
	}
	for (int i=0; i<this->units[0]; i++) {
		p[(i/size)*width + i%size] = ((w[i * this->units[1]] - min) / (max - min)) * 255.0;
	}
}
