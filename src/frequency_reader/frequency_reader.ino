/*
  CRElectrons S.A - 2019
  CRail meter
  This code is under MIT licence
  Author: Ing. Luis Leon V
*/

#define INYECTOR_SIGNAL 2
#define MINIMUM_MOTOR_CYCLE 15000 /* in us */
#define PRINTING_INTERVAL 1000000
#define FILTER_LENGTH 10

/* Global vars */
unsigned long timestamp = 0;
unsigned long last_timestamp = 0;

/*
  ------------------ Mean filter class --------------
*/

/* Filter */
class MeanFilter{
  unsigned long mean_filter_samples[FILTER_LENGTH];
  public:
    unsigned long mean_filter_output = 0;
    MeanFilter();
    bool updateFilter(unsigned long time_new_sample);
};

MeanFilter::MeanFilter(){
  for(int n = 0; n < FILTER_LENGTH; n++)
  {
    mean_filter_samples[n] = 0;
  }
}

bool MeanFilter::updateFilter(unsigned long time_new_sample){
  /* Remove bottom value */
  mean_filter_output -= mean_filter_samples[FILTER_LENGTH-1];
  /* Add new sample */
  unsigned long new_sample = time_new_sample / FILTER_LENGTH;
  mean_filter_output += new_sample;
  /* Resort samples */
  for(int n = FILTER_LENGTH - 1; n > 0; n--)
  {
    mean_filter_samples[n] = mean_filter_samples[n-1];
  }
  mean_filter_samples[0] = new_sample;
  return true;
}

/*
  ------------------ End of Mean filter class --------------
*/

unsigned long isr_new_sample = 0;
bool new_sample_available = false;

/* Filters */
MeanFilter * engineCyclesFilter;
MeanFilter * engineProcessFilter;

/* Interruption handling */
unsigned long isr_last_cycle = 0;
void sample()
{
  isr_new_sample = (timestamp - last_timestamp);
  last_timestamp = timestamp;
  new_sample_available = true;
}

void setup() {
  /* Initialise Serial for debugging */
  Serial.begin(115200);
  /* Initialise the filter */
  engineCyclesFilter = new MeanFilter();
  engineProcessFilter = new MeanFilter();
  /* Interrupt setup */
  pinMode(INYECTOR_SIGNAL, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INYECTOR_SIGNAL), sample, RISING);
}

unsigned long last_print = 0;
void print_data(){
  if(timestamp - last_print > PRINTING_INTERVAL){
    last_print = timestamp;
    Serial.print("C:");
    Serial.println(engineCyclesFilter->mean_filter_output);
    Serial.print("P:");
    Serial.println(engineProcessFilter->mean_filter_output);
  }
}

void loop() {
  /* Update value */
  if(new_sample_available){
    /* Engine cycle */
    if(isr_new_sample > MINIMUM_MOTOR_CYCLE){
      isr_new_sample = timestamp - isr_last_cycle;
      isr_last_cycle = timestamp;
      engineCyclesFilter->updateFilter(isr_new_sample);
    } else{
      engineProcessFilter->updateFilter(isr_new_sample);
    }
    new_sample_available = false;
  }
  /* Print data for debug */
  print_data();
  /* Update timestamp */
  timestamp = micros();
}
