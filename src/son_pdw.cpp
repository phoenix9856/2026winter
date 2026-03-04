#define PDW_RESERVEDDATESIZE 50//
#define PHASEANDPANUM 24//

#pragma pack (push,1)

struct SP_PDW
{
    //信号类型
    unsigned char           signalType;
    //toa;单位:ns
    unsigned long long      toa;
    //平台航向;单位:0.01°;车头角
    short platformHeading;
    //伺服角;单位:0.01°;
    short servoAngle;
    //平台修正俯仰角;单位:0.01°;
    short platformEl;
    //方位角;单位:0.01°;000-36000
    unsigned int            doa;
    //俯仰角;单位:0.01°
    int                     el;
    //频率最小值(kHz);单位:khz;
    float                   freqMin;
    //频率最大值(kHz);单位:khz;
    float                   freqMax;    
    //脉宽;单位:1ns
    unsigned int            pw;
    //功率;单位:dbm
    float                   pa;
    //脉内类型
    unsigned char           intraPulseType;
    //本振;单位: kHz
    unsigned int            freqLocal;
    //批号
    unsigned int            batchID;
    //相位信息类型 1:表示相位差；0:表示相位值
    unsigned char           phaseType;
    //相位信息通道数
    unsigned char           phaseChoNum;
    //相位信息
    float                   phases[PHASEANDPANUM];
    //幅度信息类型 1:表示幅度差；0:表示幅度值
    unsigned char           paType;
    //幅度信息通道
    unsigned char           paChoNum;
    //幅度信息
    float                   pas[PHASEANDPANUM];
    //预留
    unsigned char           reservedDate[PDW_RESERVEDDATESIZE];
    SP_PDW()
    {
        memset(this, 0, sizeof(SP_PDW));
    }
};

Q_DECLARE_METATYPE(SP_PDW)
//时间结构
typedef struct _DataGramTime
{
    quint8 Hour;
    quint8 Minute;
    quint8 Second;

    _DataGramTime()
    {
        Set(QTime::currentTime());
    }

    _DataGramTime(const QTime &time)
    {
        Set(time);
    }

    _DataGramTime(quint8 bHour, quint8 bMinute, quint8 bSecond)
    {
        Set(bHour, bMinute, bSecond);
    }

    //通过时、分、秒设置时间
    void Set(quint8 bHour, quint8 bMinute, quint8 bSecond)
    {
        Hour = bHour;
        Minute = bMinute;
        Second = bSecond;
    }

    //通过QTime设置时间
    void Set(const QTime &time)
    {
        Hour = static_cast<quint8>(time.hour());
        Minute = static_cast<quint8>(time.minute());
        Second = static_cast<quint8>(time.second());
    }

    _DataGramTime &operator=(const _DataGramTime &other)
    {
        memcpy(this, &other, sizeof(*this));
        return *this;
    }

    QTime toTime() const
    {
        QTime result(Hour, Minute, Second);
        if(result.isValid())
            return result;
        return QTime(0, 0, 0);
        //      quint8 tmpHour = Hour;
        //      quint8 tmpMinute = Minute;
        //      quint8 tmpSecond = Second;
        //      if(tmpHour > 23)
        //          tmpHour = 23;
        //      if(tmpMinute > 59)
        //          tmpMinute = 59;
        //      if(tmpSecond > 59)
        //          tmpSecond = 59;
        //      return QTime(tmpHour, tmpMinute, tmpMinute);
    }
    QString toString() const
    {
        return toTime().toString("hh:mm:ss");
    }
} DATA_GRAM_TIME;
Q_DECLARE_METATYPE(DATA_GRAM_TIME)

//与报文匹配的子PDW字结构
typedef struct sonPdwDataGram {
    DATA_GRAM_TIME		Time;               //时间（时、分、秒）
    qint32				TOA;                //相对到达时间（单位：ns）
    qint32				Toe;                //截止时间（单位：ns）
    uchar            SignalType;  //信号类型及标识
    //===hjw 20230328 新增========================
    qint32				CenterFreq;//本振值	LO	4字节	0.1MHz	0~232
    uchar        Cho_rise;  //通道号
    uchar        FrCode_rise;  //脉冲前沿频率
    qint16        FineCode_rise;//精测频校码
    uchar        Cho_fall;  //脉冲后沿频率通道号
    uchar        FrCode_fall;  //脉冲后沿频率
    qint16        FineCode_fall;//精测频校码
    qint16        PA;                 //幅度
    qint16        DOA_G3;             //方位
    qint16        EL_G4;              //俯仰
    uchar        IntraPulseMode[17];  //脉内信号类型

    unsigned char   PhaseType_G6 : 1;     //相位信息类型,“1”，表示相位差；“0”表示相位值
    unsigned char   Phase_ChoNum_G6 : 7;  //相位信息通道数pha_N
    uchar                    PhaseUnitScale_G6;          //相位码单位系数，单位：0.01度
    qint16          PhaseList_G6[8];               //相位信息，实际值 = 传输值 * PhaseUnitScale * 0.01度


    unsigned char   PaType_G7 : 1;        //幅度信息类型,“1”，表示幅度差；“0”表示幅度值
    unsigned char   Pa_ChoNum_G7 : 7;     //相位信息通道数pha_N
    qint16           PaList_G7[8];          //幅度信息
    uchar        Reserve[6];  //预留N，表示后面还有N个字节的预留

} SON_PDW_DATA_GRAM;
Q_DECLARE_METATYPE(SON_PDW_DATA_GRAM)


#pragma pack (pop)
//解硬件上传的子PDW
void MainFSYControl::GetsonPDWDataFromHandware(char *data, int len)
{
    //需要包含报文头(5字节)、信息类别号(2字节)、流水号(2字节)、GRI(2字节)、传输结束标志(1字节)
    quint8      Flag = *(reinterpret_cast<quint8*>(data + 0));       //报文头标识符,固定为字符0x7e
    quint8      ModuleID = *(reinterpret_cast<quint8*>(data + 1));   //模块代号
    quint8      UnitID = *(reinterpret_cast<quint8*>(data + 2));     //单元代号
    quint16     TotalLen = *(reinterpret_cast<quint16*>(data + 3));   //报文长度(总字节数)，包括报文头、报文体和校验位
    quint16 InfoClass = *(reinterpret_cast<quint16*>(data + 5));          //信息类别号
    quint16 SerialNO = *(reinterpret_cast<quint16*>(data + 7));           //流水号
    
    quint16 Gri = *(reinterpret_cast<const quint16*>(data + 9));//GRI标识，默认给了G1 G2 G3 G5 G6 G7 G8,所以解析下面数据时，没做GRI的判断

    int iOverFlag = *(reinterpret_cast<quint8*>(data + 11));//传输是否结束标志

    int iCountTime = *(reinterpret_cast<quint8*>(data + 12));//计数时钟

    DATA_GRAM_DATE      Date_G1 = *(reinterpret_cast<DATA_GRAM_DATE*>(data + 13));      //年日
    //qDebug()<<"MainFSYControl::GetsonPDWDataFromHandware";
    SON_PDW_DATA_GRAM *pSON_PDW_DATA_GRAM = reinterpret_cast<SON_PDW_DATA_GRAM*>(data + 37);


    QVector<SP_SonPDW> info(10);
    for(int iPdwIndex = 0; iPdwIndex < 10; iPdwIndex++, pSON_PDW_DATA_GRAM++){
        SP_SonPDW &spdw = info[iPdwIndex];

        ///时分秒
        spdw.toa = static_cast<qint64>(pSON_PDW_DATA_GRAM->TOA/ 150.0 * 1000);
        spdw.toe = static_cast<qint64>(pSON_PDW_DATA_GRAM->Toe/ 150.0 * 1000);

        //计算年日耗时太大，改成以下写法
        //        QDate curDate=QDate(2000+Date_G1.Year,1,1);
        //        unsigned long long timeS=QDateTime(curDate.addDays(Date_G1.Day),
        //          QTime(pSON_PDW_DATA_GRAM->Time.Hour,pSON_PDW_DATA_GRAM->Time.Minute,pSON_PDW_DATA_GRAM->Time.Second)).toSecsSinceEpoch()*1000000000;
        memcpy(&spdw.reservedDate[0], &Date_G1.Year, 3); //把年日放在预留字段
        spdw.reservedDate[3] = pSON_PDW_DATA_GRAM->Reserve[1];//阵面信息
        unsigned long long timeS = static_cast<unsigned long long>(pSON_PDW_DATA_GRAM->Time.Hour * 3600 + pSON_PDW_DATA_GRAM->Time.Minute * 60 + pSON_PDW_DATA_GRAM->Time.Second)* 1000000000;
        spdw.toa += timeS;
        spdw.toe += timeS;

        ///连续波标识
        spdw.isCW = pSON_PDW_DATA_GRAM->SignalType & 1;

        ///通道号
        spdw.channelNo = (pSON_PDW_DATA_GRAM->SignalType >> 4) & 0x0f;

        ///本振
        spdw.freqLocal = (0x7FFFFFFF & pSON_PDW_DATA_GRAM->CenterFreq) / 10;

        ///1-18  前开后闭
        bool bIsLowLocalFreqMode = !(0xF0000000 & pSON_PDW_DATA_GRAM->CenterFreq);

        ///前通道号、前频率码、前精测频校码
        spdw.signChannelRise = pSON_PDW_DATA_GRAM->Cho_rise;
        spdw.freqCodeRise = pSON_PDW_DATA_GRAM->FrCode_rise;
        spdw.freqCalRise= pSON_PDW_DATA_GRAM->FineCode_rise;
        unsigned int iFreq = static_cast<uint>(getFreq(spdw.freqLocal * 10,
                                                       spdw.signChannelRise, spdw.freqCodeRise, spdw.freqCalRise, bIsLowLocalFreqMode) * 1000);
        spdw.freqRise = iFreq;

        ///后通道号、后频率码、后精测频校码
        spdw.signChannelFall = pSON_PDW_DATA_GRAM->Cho_fall;
        spdw.freqCodeFall = pSON_PDW_DATA_GRAM->FrCode_fall;
        spdw.freqCalFall = pSON_PDW_DATA_GRAM->FineCode_fall;
        iFreq = static_cast<uint>(getFreq(spdw.freqLocal * 10,
                                          spdw.signChannelFall, spdw.freqCodeFall, spdw.freqCalFall, bIsLowLocalFreqMode) * 1000);
        spdw.freqFall = iFreq;

        ///功率
        spdw.pa = pSON_PDW_DATA_GRAM->PA;

        ///方位角 俯仰角
        spdw.doa = 0xffff;
        spdw.el = 0xffff;

        //分选出来的pdw没有相位信息和幅度信息，将相位信息和幅度信息打入预留字节
        ///相位信息
        spdw.phaseType = pSON_PDW_DATA_GRAM->PhaseType_G6;
        spdw.phaseChoNum = pSON_PDW_DATA_GRAM->Phase_ChoNum_G6;
        for(int iPhasePaIndex = 0; iPhasePaIndex < 8; iPhasePaIndex++){
            spdw.phases[iPhasePaIndex] = pSON_PDW_DATA_GRAM->PhaseList_G6[iPhasePaIndex] * 1.0;
            //            spdw.reservedDate[iPhasePaIndex+4] = pSON_PDW_DATA_GRAM->PhaseList_G6[iPhasePaIndex] * 1.0; //
        }

        ///幅度信息
        spdw.paType = pSON_PDW_DATA_GRAM->PaType_G7;
        spdw.paChoNum = pSON_PDW_DATA_GRAM->Pa_ChoNum_G7;
        for(int iPhasePaIndex = 0; iPhasePaIndex < 8; iPhasePaIndex++){
            spdw.pas[iPhasePaIndex] = pSON_PDW_DATA_GRAM->PaList_G7[iPhasePaIndex] * 1.0;
            //            spdw.reservedDate[iPhasePaIndex+12] =pSON_PDW_DATA_GRAM->PaList_G7[iPhasePaIndex] * 1.0;
        }
    }


    SendsonPDWToAlgorithm(info,info.size());//将子pdw发给融合算法
}


