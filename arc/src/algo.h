void pme_init(void);
uint32_t pme_process_sample(uint8_t *data, uint32_t len, uint8_t *vector);
uint16_t pme_learn(uint8_t *vector, uint32_t len, uint16_t category); 
uint16_t pme_classify(uint8_t *vector, uint32_t len);
uint16_t CuriePME_classify_all(uint8_t *pattern_vector, int32_t vector_length,
	uint16_t *distance, uint16_t *nid);
void pme_read(void);

