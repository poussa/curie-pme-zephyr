#ifdef __ZEPHYR__
#include <zephyr.h>
#include <CuriePME.h>
#else
#include <stdint.h>
#endif

#include <stdio.h>

#define SAMPLE_BUFFER_SIZE 2048
#define VECTOR_SIZE 128

static uint8_t sample_buffer[SAMPLE_BUFFER_SIZE];

static int sample_i = 0;

static uint32_t values_per_sample;
static uint32_t samples_per_vector;

static uint8_t average(uint8_t *input, uint32_t pos, uint32_t step, uint32_t count)
{
	uint32_t ret = 0;

	//printf("%s: pos=%3d, step=%3d, count=%3d", __FUNCTION__, pos, step, count);

	for (int i = 0; i < count; i++) {
		ret += input[pos + (step * i)];
	}
	ret /= count;
	//printf(", ret=%3d\n", ret);

	return (uint8_t)ret;
}

static void undersample(uint8_t *input, uint32_t samples, uint8_t *output)
{
	uint32_t ii = 0; // input position
	uint32_t oi = 0; // outout position
	uint32_t count = samples / samples_per_vector;

	printf("%s: samples=%lu samples_per_vector=%ld count=%ld\n", 
		__FUNCTION__, samples, samples_per_vector, count);

	for (int i = 0; i < samples_per_vector; i++) {
		for (int j = 0; j < values_per_sample; j++) {
			output[oi + j] = average(input, ii + j, values_per_sample, count);
		}
		ii += (count * 3);
		oi += 3;
	}
}

void pme_init(void)
{
	values_per_sample  = 3; // X,Y,Z
	samples_per_vector = VECTOR_SIZE / values_per_sample;
#ifdef __ZEPHYR__
	printf("%s\n", __FUNCTION__);
	CuriePME_begin();
	CuriePME_configure(1, L1_Distance, RBF_Mode, 0, 32);
#endif
}

uint32_t pme_process_sample(uint8_t *data, uint32_t data_len, uint8_t *vector)
{
	sample_buffer[sample_i] = data[0];
	sample_buffer[sample_i + 1] = data[1];
	sample_buffer[sample_i + 2] = data[2];

	sample_i += 3;

	if (sample_i + 3 > SAMPLE_BUFFER_SIZE) {
		undersample(sample_buffer, sample_i/3, vector);
		sample_i = 0;
		return 1;
	}
	return 0;
}

uint16_t pme_learn(uint8_t *vector, uint32_t len, uint16_t category) 
{
	printf("%s: category=%d is %lu byte vector\n", __FUNCTION__, category, len);
	for (int i = 0; i < len; i++)
		printf("%d ", vector[i]);
	printf("\n");
	return CuriePME_learn(vector, len, category);
}

uint16_t pme_classify(uint8_t *vector, uint32_t len) 
{
	printf("%s: %lu byte vector\n", __FUNCTION__, len);
	for (int i = 0; i < len; i++)
		printf("%d ", vector[i]);
	printf("\n");

	CuriePME_bcast_vector(vector, len);

	uint16_t dist=0, id=0, cat, bestCat;

	bestCat = cat = CuriePME_classify_next(&dist, &id);
	while (cat != 0x7FFF) {
		printf("pme_classify: cat=%d dist=%d id=%d\n", cat, dist, id);
		cat = CuriePME_classify_next(&dist, &id);
	}
	return bestCat;
}

void pme_read(void)
{
	uint16_t count;
	neuronData neuron;

	printf("context: %d\n", CuriePME_getGlobalContext());
	printf("neurons: %d\n", CuriePME_getCommittedCount());
	CuriePME_beginSaveMode();
	
	while ((count = CuriePME_iterateNeuronsToSave(&neuron)) != 0) {
		if (count == 0x7FFF)
			break;
		printf("Neuron: NID=%d %s CTX=%d AIF=%d MIF=%d cat=%d\n", 
			neuron.context & NCR_ID,
			(neuron.context & NCR_NORM) == 0 ? "L1" : "LSUP",
			neuron.context & NCR_CONTEXT, 
			neuron.influence,
			neuron.minInfluence, 
			neuron.category);
	}

	CuriePME_endSaveMode();
}

#ifndef __ZEPHYR__
void fill(uint16_t *data, uint16_t v0, uint16_t v1, uint16_t v2)
{
	data[0] = v0; data[1] = v1; data[2] = v2;
}

int main()
{
	uint16_t test[3];
	int i;

	pme_init();

	FILE *raw_file = fopen("raw.txt", "w");
	FILE *scale_file = fopen("scale.txt", "w");
	FILE *vector_file = fopen("vector.txt", "w");

	if (!raw_file || !scale_file || !vector_file) {
		printf("Can not open file\n");
		return 0;
	}

	for (i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
		fill(test, i*100, i*200, i*300);
		fprintf(raw_file, "%5d %5d %5d %5d\n", i, test[0], test[1], test[2]);
		if (pme_process_sample(test, sizeof(test))) {
			break;
		}
	}

	for (i = 0; i < SAMPLE_BUFFER_SIZE; i += 3)
		fprintf(scale_file, "%5d,%5d,%5d,%5d\n", i/3, sample_buffer[i], sample_buffer[i+1], sample_buffer[i+2]);
	
	for (i = 0; i < VECTOR_SIZE; i += 3)
		fprintf(vector_file, "%5d,%5d,%5d,%5d\n", i/3, vector[i], vector[i+1], vector[i+2]);

	fclose(raw_file);
	fclose(scale_file);
	fclose(vector_file);
}
#endif