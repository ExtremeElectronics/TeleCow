
#define SOUNDBUFFMAX 512 // Size of sound buffers (needs to be greater than SamplesPerPacket


#define SamplesPerSecond 8000 //hz
#define SampleTime 125 //uS
#define SamplesPerPacket 160
#define PacketsPerSecond 50

uint16_t fring(uint16_t s_in);
//uint8_t ftext(uint8_t s_in);
void srandfrommike(void);
void SetPWM(void);
void SetA2D(void);
void ZeroMikeBuffer(void);
void ZeroSpkBuffer(void);
void AddToSpkBuffer(uint8_t v);
void AddToMikeBuffer(uint8_t v);
uint8_t GetFromSpkBuffer(void);
bool Sound_Timer_Callback(struct repeating_timer *t);
void init_sound(void);
void send_sound(void);
void StartSoundTimer(void);

