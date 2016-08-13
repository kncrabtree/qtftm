#include "amdorbatch.h"

AmdorBatch::AmdorBatch(QList<QPair<Scan, bool> > templateList, QList<QPair<double,double>> drOnlyList, double threshold, double fw, double exc, int maxChildren, AbstractFitter *ftr) : BatchManager(QtFTM::Amdor,false,ftr),
    d_currentFtIndex(0), d_currentDrIndex(0), d_currentRefInt(0.0), d_threshold(threshold), d_frequencyWindow(fw),
    d_excludeRange(exc), d_maxChildren(maxChildren), d_calIsNext(false), d_currentScanIsRef(false),
    d_currentScanIsVerification(false)
{
    //build completed and scan matrices
    //in the scan matrix, diagonal elements are reference scans.
    //Assuming the user organized the template list in decreasing cavity intensity,
    //If ftIndex < drIndex, the DR test is looking at the stronger line in the cavity
    //The other situation, drIndex < ftIndex, may only be used for verification of a match
    int totalTests = 0;
    d_hasCal = false;
    d_scansSinceCal = 0;
    p_currentNode = nullptr;
    for(int i=0; i<templateList.size(); i++)
    {
        if(templateList.at(i).second)
        {
            d_hasCal = true;
            d_calScanTemplate = templateList.at(i).first;
            continue;
        }

        QList<bool> cList;
        QList<Scan> sList;
        d_frequencies.append(templateList.at(i).first.ftFreq());

        QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
        s.beginGroup(QString("drSynth"));
        s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
        double minDr = s.value(QString("min")).toDouble();
        double minPwr = s.value(QString("minPower"),-20.0).toDouble();
        minDr += 648.21;
        s.endGroup();
        s.endGroup();

        for(int j=0; j<templateList.size(); j++)
        {
            if(templateList.at(j).second)
                continue;


            Scan si = templateList.at(i).first;
            Scan sj = templateList.at(j).first;


            Scan thisScan = si;
            PulseGenConfig pg = thisScan.pulseConfiguration();
            if(j == i)
            {
                //this is a reference scan; disable DR
                pg.setDrEnabled(false);
                thisScan.setDrPower(minPwr);
                thisScan.setDrFreq(minDr);
            }
            else if(j < i)
            {
                //switch ft and dr frequencies; use number of shots from weaker scan
                thisScan = sj;
                pg.setDrEnabled(true);
                thisScan.setDrFreq(si.ftFreq());
                thisScan.setDrPower(si.drPower());
            }
            else
            {
                pg.setDrEnabled(true);
                thisScan.setDrFreq(sj.ftFreq());
                thisScan.setDrPower(sj.drPower());
                totalTests++;
            }

            thisScan.setPulseConfiguration(pg);
            sList.append(thisScan);

            //if the two frequencies fall within exclude range, mark as complete to skip
            if(qAbs(thisScan.ftFreq() - thisScan.drFreq()) < d_excludeRange && i != j)
            {
                cList.append(true);
                if(j > i)
                    totalTests--;
            }
            else
                cList.append(false);

        }

        //add on DR only scans
        for(int j=0; j<drOnlyList.size(); j++)
        {

            Scan si = templateList.at(i).first;
            si.setDrFreq(drOnlyList.at(j).first);
            si.setDrPower(drOnlyList.at(j).second);
            PulseGenConfig pg = si.pulseConfiguration();
            pg.setDrEnabled(true);
            si.setPulseConfiguration(pg);
            sList.append(si);

            if(qAbs(si.ftFreq() - si.drFreq()) < d_excludeRange)
                cList.append(true);
            else
            {
                cList.append(false);
                totalTests++;
            }

        }

        d_completedMatrix.append(cList);
        d_scanMatrix.append(sList);

    }

    //make sure DR only frequencies are added to all frequencies list
    for(int i=0; i<drOnlyList.size(); i++)
        d_frequencies.append(drOnlyList.at(i).first);

    d_currentFtIndex = 0;
    d_currentDrIndex = 0;
    if(d_hasCal)
    {
        d_calIsNext = true;
        totalTests++;
    }

    d_totalShots = totalTests;
}

AmdorBatch::AmdorBatch(int num, AbstractFitter *ftr) :
    BatchManager(QtFTM::Amdor,true,ftr), d_currentFtIndex(0), d_currentDrIndex(0), d_currentRefInt(0.0),
    d_calIsNext(false), d_currentScanIsRef(false), d_currentScanIsVerification(false)
{

    d_batchNum = num;

    int batchMillions = d_batchNum/1000000;
    int batchThousands = d_batchNum/1000;

    //create directory, if necessary
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    QString savePath = s.value(QString("savePath"),QString(".")).toString();
    QDir d(savePath + QString("/amdor/%1/%2").arg(batchMillions).arg(batchThousands));
    QFile in(QString("%1/%2.txt").arg(d.absolutePath()).arg(d_batchNum));

    d_threshold = 0.5;
    d_frequencyWindow = 0.1;
    d_loadIndex = 0;
    d_scansSinceCal = 0;
    d_excludeRange = 100.0;
    d_maxChildren = 3;

    if(in.open(QIODevice::ReadOnly))
    {
        while(!in.atEnd())
        {
            QByteArray line = in.readLine().trimmed();
            QByteArrayList l = line.split('\t');
            bool ok = false;

            if(line.isEmpty())
                continue;

            if(line.startsWith("#Match threshold"))
            {
                if(l.size() < 2)
                    continue;

                double t = l.at(1).trimmed().toDouble(&ok);
                if(ok && t > 0.0 && t < 1.0)
                    d_threshold = t;

                continue;
            }

            if(line.startsWith("#Frequency window"))
            {
                if(l.size() < 2)
                    continue;

                double w = l.at(1).trimmed().toDouble(&ok);
                if(ok && w > 0.0 && w < 1.0)
                    d_frequencyWindow = w;

                continue;
            }

            if(line.startsWith("#Exclude range"))
            {
                if(l.size() < 2)
                    continue;

                double w = l.at(1).trimmed().toDouble(&ok);
                if(ok)
                    d_excludeRange = w;

                continue;
            }

            if(line.startsWith("#Max children"))
            {
                if(l.size() < 2)
                    continue;

                double w = l.at(1).trimmed().toInt(&ok);
                if(ok && w > 0)
                    d_maxChildren = w;

                continue;
            }

            if(line.startsWith("amdorfrequencies"))
                break;
        }

        //read in frequency list
        QList<bool> drOnly;
        while(!in.atEnd())
        {
            QByteArray line = in.readLine().trimmed();
            if(line.startsWith("amdor"))
                break;

            QByteArrayList l = line.split('\t');
            bool ok = false;
            if(line.isEmpty() || l.size() < 2)
                continue;

            double f = l.at(0).toDouble(&ok);
            if(!ok)
                continue;

            bool b = static_cast<bool>(l.at(1).toInt(&ok));
            if(!ok)
                continue;

            d_frequencies.append(f);
            drOnly.append(b);

        }

        //read in scan information
        while(!in.atEnd())
        {
            QByteArray line = in.readLine().trimmed();
            QByteArrayList l = line.split('\t');
            bool ok = false;
            if(line.isEmpty() || l.size() < 7)
                continue;

            int sn = l.takeFirst().toInt(&ok);
            if(!ok)
                continue;

            bool isCal = static_cast<bool>(l.takeFirst().toInt(&ok));
            if(!ok)
                continue;

            bool isRef = static_cast<bool>(l.takeFirst().toInt(&ok));
            if(!ok)
                continue;

            bool isVer = static_cast<bool>(l.takeFirst().toInt(&ok));
            if(!ok)
                continue;

            int ftId = l.takeFirst().toInt(&ok);
            if(!ok)
                continue;

            int drId = l.takeFirst().toInt(&ok);
            if(!ok)
                continue;

            //don't need the intensity, but it comes here

            d_loadScanList.append(sn);
            d_loadCalList.append(isCal);
            d_loadRefList.append(isRef);
            d_loadVerificationList.append(isVer);
            d_loadIndices.append(qMakePair(ftId,drId));

        }

    }

    in.close();

}

AmdorBatch::~AmdorBatch()
{
    while(!d_trees.isEmpty())
        delete d_trees.takeFirst();
}

QList<double> AmdorBatch::allFrequencies()
{
    return d_frequencies;
}

double AmdorBatch::matchThreshold()
{
    return d_threshold;
}



void AmdorBatch::writeReport()
{
    if(d_saveData.isEmpty())
    {
        emit logMessage(QString("Did not create AMDOR report because no scans were completed."),QtFTM::LogWarning);
        return;
    }

    //figure out where to save the data
    QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
    int batchNum = s.value(d_numKey,1).toInt();
    int batchMillions = batchNum/1000000;
    int batchThousands = batchNum/1000;

    //create directory, if necessary
    QString savePath = s.value(QString("savePath"),QString(".")).toString();
    QDir d(savePath + QString("/amdor/%1/%2").arg(batchMillions).arg(batchThousands));
    if(!d.exists())
    {
        if(!d.mkpath(d.absolutePath()))
        {
            emit logMessage(QString("Could not create directory for saving AMDOR data! Creation of %1 failed, and data were not saved!").arg(d.absolutePath()),QtFTM::LogError);
            return;
        }
    }

    //open file for writing
    QFile out(QString("%1/%2.txt").arg(d.absolutePath()).arg(batchNum));
    QTextStream t(&out);
    QString tab = QString("\t");
    QString nl = QString("\n");

    if(!out.open(QIODevice::WriteOnly))
    {
        emit logMessage(QString("Could not open file for writing AMDOR data! Creation of %1 failed, and data were not saved!").arg(out.fileName()),QtFTM::LogError);
        return;
    }

    //header stuff
    t << QString("#AMDOR") << tab << batchNum << tab << nl;
    t << QString("#Date") << tab << QDateTime::currentDateTime().toString() << tab << nl;
    t << QString("#FID delay") << tab << d_fitter->delay() << tab << QString("us") << nl;
    t << QString("#FID high pass") << tab << d_fitter->hpf() << tab << QString("kHz") << nl;
    t << QString("#FID exp decay") << tab << d_fitter->exp() << tab << QString("us") << nl;
    t << QString("#FID remove baseline") << tab << (d_fitter->removeDC() ? QString("Yes") : QString("No")) << tab << nl;
    t << QString("#FID zero padding") << tab << (d_fitter->autoPad() ? QString("Yes") : QString("No")) << tab << nl;
    t << QString("#Match threshold") << tab << QString::number(d_threshold,'f',3) << tab << nl;
    t << QString("#Frequency window") << tab << QString::number(d_frequencyWindow,'f',3) << tab << QString("MHz") << nl;
    t << QString("#Exclude range") << tab << QString::number(d_excludeRange,'f',3) << tab << QString("MHz") << nl;
    t << QString("#Max children") << tab << QString::number(d_maxChildren) << tab << nl;

    t << nl <<QString("amdorfrequencies") << batchNum << tab << QString("amdordronly") << batchNum;
    t.setRealNumberNotation(QTextStream::FixedNotation);
    t.setRealNumberPrecision(3);
    for(int i=0; i<d_frequencies.size(); i++)
        t << nl << d_frequencies.at(i) << tab << (i > d_completedMatrix.size() ? 1 : 0);

    t << nl << nl;
    t << QString("amdorscan") << batchNum << tab << QString("amdoriscal") << batchNum;
    t << tab << QString("amdorisref") << batchNum << tab << QString("amdorisval") << batchNum << tab;
    t << QString("amdorftid") << batchNum;
    t << tab << QString("amdordrid") << batchNum << tab << QString("amdorintensity") << batchNum;

    for(int i=0; i<d_saveData.size(); i++)
    {
        const AmdorSaveData &sd = d_saveData.at(i);
        t << nl << sd.scanNum;
        t << tab << (sd.isCal ? 1 : 0);
        t << tab << (sd.isRef ? 1 : 0);
        t << tab << (sd.isValidation ? 1 : 0);
        t << tab << sd.ftId;
        t << tab << sd.drId;
        t << tab << sd.intensity;
    }

    s.setValue(d_numKey,batchNum+1);
    s.sync();

}

void AmdorBatch::advanceBatch(const Scan s)
{
    //if loading, get information from lists
    if(d_loading)
    {
        d_thisScanIsCal = d_loadCalList.at(d_loadIndex);
        d_currentScanIsRef = d_loadRefList.at(d_loadIndex);
        d_currentScanIsVerification = d_loadVerificationList.at(d_loadIndex);
        d_currentFtIndex = d_loadIndices.at(d_loadIndex).first;
        d_currentDrIndex = d_loadIndices.at(d_loadIndex).second;
    }

    //processing needs to be done here because the result may decide what scan to perform next
    d_lastScan = s;
    d_scansSinceCal++;

    FitResult res = d_fitter->doFit(s);

    //the scan number will be used on the X axis of the plot
    double num = (double)s.number();

    QPair<QVector<QPointF>,double> p = d_fitter->doStandardFT(s.fid());
    QVector<QPointF> ft = p.first;
    double max = p.second;
    double intensity = max;

    //Get relevant intensity information... will be used later for analysis
    if(res.type() == FitResult::DopplerPair)
    {
        intensity = -1.0;
        QList<QPair<double,double>> faList = res.freqAmpPairList();
        double closestFreq = 100.0;
        for(int i=0; i<faList.size(); i++)
        {
            double diff = qAbs(faList.at(i).first - s.ftFreq());
            if(diff < d_frequencyWindow && diff < closestFreq)
                intensity = faList.at(i).second;
        }
    }
    else
    {
        intensity = 0.0;
        for(int i=0; i<ft.size(); i++)
        {
            if(fabs(ft.at(i).x()-s.ftFreq()) < d_frequencyWindow)
                intensity = qMax(intensity,ft.at(i).y());
        }
    }

    AmdorSaveData sd;
    sd.scanNum = s.number();
    sd.ftId = d_currentFtIndex;
    sd.drId = d_currentDrIndex;
    sd.intensity = intensity;
    sd.isCal = d_thisScanIsCal;
    sd.isRef = d_currentScanIsRef;
    sd.isValidation = d_currentScanIsVerification;

    d_saveData.append(sd);

    /****************************************
     * INFORMATION FOR BATCH PLOT
     * **************************************/

    QString markerText;
    QTextStream t(&markerText);

    //the data will start and end with a 0 to make the plot look a little nicer
    if(!d_thisScanIsCal)
    {
        d_drData.append(QPointF(num - ((double)ft.size()/2.0 + 1.0)/(double)ft.size()*0.9,0.0));
        for(int i=0; i<ft.size(); i++) // make the x range go from num - 0.45 to num + 0.45
            d_drData.append(QPointF(num - ((double)ft.size()/2.0 - (double)i)/(double)ft.size()*0.9,ft.at(i).y()));
        d_drData.append(QPointF(num - ((double)ft.size()/2.0 - (double)ft.size())/(double)ft.size()*0.9,0.0));
    }
    else
    {
        t << QString("CAL\n");
        d_calData.append(QPointF(num,intensity));
    }

    //show ftm frequency and attenuation
    t << QString("ftm:") << QString::number(s.ftFreq(),'f',1) << QString("/") << s.attenuation();

    double mdmin = static_cast<double>(s.number());
    double mdmax = static_cast<double>(s.number());
    bool badTune = s.tuningVoltage() <= 0;

    if(!d_thisScanIsCal)
    {
        //if DR is on, show DR freq and power
        if(s.pulseConfiguration().isDrEnabled())
            t  << QString("\n") << QString("dr:") << QString::number(s.drFreq(),'f',1)
               << QString("/") << QString::number(s.drPower(),'f',1);
        t << QString("\n");
        //show pulse configuration
        for(int i=0;i<s.pulseConfiguration().size();i++)
        {
            if(s.pulseConfiguration().at(i).enabled)
                t << 1;
            else
                t << 0;
        }

        mdmin = num - ((double)ft.size()/2.0 + 1.0)/(double)ft.size()*0.9;
        mdmax = num - ((double)ft.size()/2.0 - (double)ft.size())/(double)ft.size()*0.9;
    }

    if(d_currentScanIsRef)
        t << QString("\n") << QString("REF");

    t.flush();

    QtFTM::BatchPlotMetaData md(QtFTM::Amdor,s.number(),mdmin,mdmax,d_thisScanIsCal,badTune,markerText);
    md.isRef = d_currentScanIsRef;

    bool link = false;
    if(!d_thisScanIsCal)
    {
        if(d_currentScanIsRef)
            d_currentRefInt = intensity;
        else
            link = intensity < (d_threshold*d_currentRefInt);
    }
    md.drMatch = link;

    //send the data to the plot
    QList<QVector<QPointF> > out;
    if(!d_thisScanIsCal)
        out.append(d_drData);
    else
        out.append(d_calData);

    emit plotData(md,out);

    /****************************************
     * INFORMATION FOR AMDOR DATA AND BATCH *
     * **************************************/
    ///TODO: Implement verification?

    //When loading, only send data to AmdorWidget
    if(d_loading)
    {
        if(d_currentScanIsRef)
            emit newRefScan(s.number(),d_currentFtIndex,intensity);
        else if(!d_thisScanIsCal && !d_currentScanIsVerification)
            emit newDrScan(s.number(),d_currentDrIndex,intensity);

        return;
    }

    if(d_thisScanIsCal)
    {
        d_scansSinceCal = 0;
        d_calIsNext = false;
    }
    else if(d_currentScanIsRef)
    {
        emit newRefScan(s.number(),d_currentFtIndex,intensity);
        if(!d_completedMatrix.at(d_currentFtIndex).at(d_currentDrIndex))
            d_completedMatrix[d_currentFtIndex][d_currentDrIndex] = true;

        //find next undone DR scan for this FT frequency
        //if we are doing a ref scan, there must be at least one remaining DR test to perform.
        //find the next one
        for(int i=d_currentFtIndex+1; i<d_completedMatrix.at(d_currentFtIndex).size(); i++)
        {
            if(!d_completedMatrix.at(d_currentFtIndex).at(i))
            {
                d_currentDrIndex = i;
                break;
            }
        }

        if(intensity < 0.0)
        {
            //no detection; ignore this test and move on
            for(int i=d_currentFtIndex; i<d_completedMatrix.at(d_currentFtIndex).size(); i++)
                d_completedMatrix[d_currentFtIndex][i] = true;

            if(nextTreeBranch())
            {
                //we will need to first do a reference scan
                d_currentFtIndex = p_currentNode->freqIndex;
                d_currentDrIndex = p_currentNode->freqIndex;
            }
            else
                resumeFromBranch();
        }
        else
            d_currentRefInt = intensity;
    }
    else
    {
        d_completedMatrix[d_currentFtIndex][d_currentDrIndex] = true;
        if(d_currentDrIndex > d_currentFtIndex)
        {
            //how to handle verification? Probably cache results and wait to emit these until
            //after all verification tests are complete
            emit advanced();
            emit newDrScan(s.number(),d_currentDrIndex,intensity);
        }

        //we have completed a DR test.
        //There are several possibilities for our next step
        //Since the goal is to build linked networks of lines as quickly as possible,
        //we make a tree structure to track the network we're currently exploring.
        //The tree is empty at first. Its root node will contain the ft freqency index
        //of the first linkage, and it will contain a child with the first DR linkage.
        //further DR linkages will be added as children to the root node.
        //After finishing with the root node, the tree structure can be descended,
        //setting the FT frequency for the scan to the former DR frequency and searching
        //for more linkages. First, sibling linkages are tested, then children are located.
        //This procedure only examines cases in which ftIndex > drIndex.

        //The program is designed to find up to d_maxChildren children at each level before descending the tree.
        //This is in an effort to balance time spent tuning the cavity against the speed at which
        //a single network is explored. When d_maxChildren children are found, or when all other possibilities
        //are exhausted, the program will descend the tree searching for up to d_maxChildren children at each level
        //Once no children are found at a level, the program will exit the tree and continue where
        //it left off before beginning the tree traversal. As it goes from there, each new link it finds
        //will be tested against all known trees, and if it is linked to the tree, the program will just
        //continue. However, if the program finds a new linkage that does not belong to any known tree,
        //it will begin the traversal algorithm again.

        //However, it is also possible to disable the tree traversal algorithm by setting d_maxChildren to 0.
        //Then the program will simply perform a normal DR correlation; each frequency in turn.


        //if this is set to true, then the next scan will be determined from p_currentNode
        bool useNodeIndices = false;
        int ftId = d_currentFtIndex;
        int drId = d_currentDrIndex;

        //check to see if this was the last DR scan for the current FT Index
        bool adv = incrementIndices();

        //If the line is not linked, then we need to move on
        if(!link)
        {
            //move to the next branch if exploring a tree and we've exhausted the current ft Index
            if(p_currentNode != nullptr && adv)
            {
                if(nextTreeBranch())
                    useNodeIndices = true;
                else
                    resumeFromBranch();
            }

        }
        else
        {
            //we've found a linkage.
            //first, check if this linkage is associated with another tree
            bool oldTree = false;
            for(int i=0; i<d_trees.size(); i++)
            {
                if(d_trees[i]->addChildToTree(ftId,drId))
                {
                    oldTree = true;
                    break;
                }
            }

            if(!oldTree)
            {
                if(p_currentNode != nullptr)
                {
                    //check to see if this linkage is a sibling
                    bool isSibling = false;
                    if(!p_currentNode->isRootNode())
                    {
                        for(int i=p_currentNode->parentIndex; i<p_currentNode->parent->children.size(); i++)
                        {
                            if(drId == p_currentNode->parent->children.at(i)->freqIndex)
                                isSibling = true;
                        }
                    }

                    //The linkage is associated with our current tree. Add it as a child unless it's a sibling
                    if(!isSibling)
                        p_currentNode->addChild(drId);

                    //if the children list is now at limit, OR we've done the last DR scan we can, go to next branch if possible
                    if(p_currentNode->children.size() >= d_maxChildren || adv)
                    {
                        if(nextTreeBranch())
                            useNodeIndices = true;
                    }
                }
                else
                {
                    //this is a brand new linkage for a new tree
                    //special case: if d_maxChildren is 0, don't explore tree
                    if(d_maxChildren > 0)
                    {
                        p_currentNode = new AmdorNode(ftId);
                        p_currentNode->addChild(drId);
                        //move to child if we're out of DR scans
                        if(adv)
                        {
                            if(nextTreeBranch())
                                useNodeIndices = true;
                            else
                                resumeFromBranch();
                        }
                    }

                    //if maxChildren is 0, then we will only use the indices
                    //from incrementIndices()

                }
            }
            else
            {
                //if oldTree is true, then the current tree we're exploring is linked
                //to one we've already looked at. Ditch this tree and resume
                //from the place we branched off from when we started it.
                if(p_currentNode != nullptr)
                {
                    d_trees.append(p_currentNode);
                    p_currentNode = nullptr;
                    resumeFromBranch();
                }
            }
        }

        //at this point, is we're using node indices, we set them to the appropriate values for
        //the current node. Otherwise, we just use the indices set by incrementIndices
        if(useNodeIndices)
        {
            //we will need to first do a reference scan
            d_currentFtIndex = p_currentNode->freqIndex;
            d_currentDrIndex = p_currentNode->freqIndex;
        }
        else if(adv && d_hasCal && d_scansSinceCal >= 20)
        {
            //May as well do a calibration scan here if we're done with a tree or done with a frequency
            d_calIsNext = true;
        }
    }

}

void AmdorBatch::processScan(Scan s)
{
    Q_UNUSED(s)
    if(d_loading)
        d_loadIndex++;
}

Scan AmdorBatch::prepareNextScan()
{
    if(d_hasCal && d_calIsNext)
    {
        d_calIsNext = false;
        d_thisScanIsCal = true;
        d_currentScanIsRef = false;
        d_currentScanIsVerification = false;
        return d_calScanTemplate;
    }

    //TODO: handle verification scans too!

    d_thisScanIsCal = false;
    Scan out = d_scanMatrix.at(d_currentFtIndex).at(d_currentDrIndex);
    if(d_currentFtIndex == d_currentDrIndex)
    {
        d_currentScanIsRef = true;
        d_currentScanIsVerification = false;

        QSettings s(QSettings::SystemScope,QApplication::organizationName(),QApplication::applicationName());
        s.beginGroup(QString("drSynth"));
        s.beginGroup(s.value(QString("subKey"),QString("virtual")).toString());
        double minDr = s.value(QString("min")).toDouble();
        double minPwr = s.value(QString("minPower"),-20.0).toDouble();
        minDr += 648.21;
        s.endGroup();
        s.endGroup();

        out.setDrPower(minPwr);
        out.setDrFreq(minDr);
    }
    else
    {
        d_currentScanIsRef = false;
        d_currentScanIsVerification = false;
        if(qAbs(d_lastScan.ftFreq() - out.ftFreq()) < 0.1)
            out.setSkiptune(true);
    }
    return out;
}

bool AmdorBatch::isBatchComplete()
{
    for(int i=0; i<d_completedMatrix.size(); i++)
    {
        for(int j=i+1; j<d_completedMatrix.at(i).size(); j++)
        {
            if(!d_completedMatrix.at(i).at(j))
                return false;
        }
    }

    if(d_hasCal)
    {
        if(d_calIsNext)
            return false;
        else
            emit advanced(); //this is the final scan; make sure progress goes to 100%
    }

    return true;
}

bool AmdorBatch::incrementIndices()
{
    //this function finds the next ft and dr index in the current series.
    //It returns true if the ft index changes
    for(int j=d_currentFtIndex; j < d_completedMatrix.size(); j++)
    {
        for(int i=j+1; i<d_completedMatrix.at(d_currentFtIndex).size(); i++)
        {
            if(!d_completedMatrix.at(j).at(i))
            {
                if(j == d_currentFtIndex)
                {
                    d_currentDrIndex = i;
                    return false;
                }
                else
                {
                    //need to do a reference scan
                    d_currentDrIndex = j;
                    d_currentFtIndex = j;
                    return true;
                }
            }
        }
    }

    //if we've made it here, then we need to search from the beginning
    for(int j = 0; j<d_currentFtIndex; j++)
    {
        for(int i=j+1; i<d_completedMatrix.at(d_currentFtIndex).size(); i++)
        {
            if(!d_completedMatrix.at(j).at(i))
            {
                //need to do a reference scan
                d_currentDrIndex = j;
                d_currentFtIndex = j;
                return true;
            }
        }
    }

    //if we've made it here, then all scans are complete
    d_currentDrIndex = d_completedMatrix.last().size()-1;
    d_currentFtIndex = d_completedMatrix.size()-1;
    return true;
}

bool AmdorBatch::nextTreeBranch()
{
    //this function moves to the next branch of the current tree, returning true if successful
    //if there is no next branch, return false and do not set indices
    if(p_currentNode == nullptr)
        return false;

    //check to see if each child has been tested
    for(int i=0; i<p_currentNode->children.size(); i++)
    {
        int id = p_currentNode->children.at(i)->freqIndex;

        //no tests are possible on DR only scans
        if(id >= d_completedMatrix.size())
            continue;
        if(id+1 >= d_completedMatrix.at(i).size())
            continue;


        if(!d_completedMatrix.at(id).at(id+1))
        {
            p_currentNode = p_currentNode->children.at(i);
            return true;
        }
    }

    //all children have already been tested.
    //if this is the root node, then all branches are done.
    //otherwise, go up the tree
    if(!p_currentNode->isRootNode())
    {
        p_currentNode = p_currentNode->parent;
        return nextTreeBranch();
    }

    //we're done with this node, so store the node pointer in the list and null the current handle out
    d_trees.append(p_currentNode);
    p_currentNode = nullptr;
    return false;
}

void AmdorBatch::resumeFromBranch()
{
    for(int i=0; i<d_completedMatrix.size(); i++)
    {
        for(int j=i+1; j<d_completedMatrix.at(i).size(); j++)
        {
            if(!d_completedMatrix.at(i).at(j))
            {
                //pick up where we originally branched off
                d_currentFtIndex = i;
                d_currentDrIndex = i;
                return;
            }
        }
    }
}
