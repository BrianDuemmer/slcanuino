//CanbusBase.cpp

// Base implementation of a Can Bus
class CanbusBase
{
  public:
	CanbusBase();
    virtual char init(unsigned char) = 0;
	
	// more general methods
	virtual char message_tx(tCAN *msg) = 0;
	virtual char message_rx(tCAN *msg) = 0;
private:
};