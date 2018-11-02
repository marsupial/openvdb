

class CxxThing1 {
};

class CxxThing2 {
};

struct CThing {
  static int CThingsThing;
};

int CThing::CThingsThing  = 0;

void separatefuncttion() {
  CThing::CThingsThing = 10;
  
  int a = 32;
  a = 45;
  CThing::CThingsThing = 32;
}

int gThing;

static int sThing;

const int kThing= 0;

const static int kSThing = 0;

const char* kData1 = "";
char* const kData2 = 0;
const char kData3[] = { 0, 0, 0 };



int main(int argc, const char** argv) {
  static int sLevel = 0;
  kData1 = "45";
  //kData3 = kData2;
  return 0;
}
