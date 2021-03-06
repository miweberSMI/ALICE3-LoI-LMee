R__LOAD_LIBRARY(libDelphes)
R__LOAD_LIBRARY(libDelphesO2)

bool smear = true;
bool nsigma = true;
double Bz = 0.2;
double eMass = 0.000511;

// Cinematic cuts on tracks
double PtCut = 0.04;
double EtaCut = 1.1;

void makeHistNice(TH1* h, int color){
    h->SetMarkerColor(color);
    h->SetLineColor(color);
    h->SetMarkerStyle(20);
    h->SetLineWidth(2);
}

bool kineCuts(Track *tr){
    // check pt and eta for track
    // evaluate as true if criterion is passed
    bool pt = tr->PT > PtCut;
    bool eta = abs(tr->Eta) < EtaCut;
    // all have to be true
    return (pt && eta);

}

bool isSelectedMC(Track *tr)
{
  auto particle = (GenParticle *)tr->Particle.GetObject();
  if (abs(particle->PID) == 11) return true;
  else return false;
}

bool isSelectedTOF(Track *tr)
{
  // need implementation
  return false;
}

bool isSelectedRICH(Track *tr)
{
  return false;
}

bool hasCommonAncestor(GenParticle* p1,GenParticle* tr2 )
{
    // check if 2 particles have one ancestor
    // fill vector with all ancestors of particle on
    // check if particle2 is included
    // if so, return true if not check for mother of p2
    // maybe do this recursiv???
    return false;
}

bool hasStrangeAncestor(GenParticle *particle, TClonesArray *particles)
{
    auto imother = particle->M1;
    if (imother == -1) return false;
    auto mother = (GenParticle *)particles->At(imother);
    auto pid = mother->PID;

    switch (abs(pid)) {
        case 310:  // K0s
        case 3122: // Lambda
        case 3112: // Sigma-
        case 3222: // Sigma+
        case 3312: // Xi-
        case 3322: // Xi0  was not included by Roberto in his example
        case 3334: // Omega-
            return true;
    }

    return hasStrangeAncestor(mother, particles);
}

bool hasHeavyAncestor(GenParticle *particle, TClonesArray *particles)
{
    auto imother = particle->M1;
    if (imother == -1) return false;
    auto mother = (GenParticle *)particles->At(imother);
    auto pid = mother->PID;

    switch (abs(pid)) {
        case 411:  // D+
        case 421:  // D0
        case 431:  // Ds+
        case 4122: // Lambdac+
        case 4132: // Xic0
        case 4232: // Xic+
        case 511:  // B0
        case 521:  // B+
        case 531:  // Bs0
        case 541:  // Bc+
        case 5122: // Lambdab0
        case 5132: // Xib-
        case 5232: // Xib0
        case 5332: // Omegab-
            return true;
    }

    return hasHeavyAncestor(mother, particles);
}


bool isBeauty(int pid){
    switch (abs(pid)) {
        case 511:  // B0
        case 521:  // B+
        case 531:  // Bs0
        case 541:  // Bc+
        case 5122: // Lambdab0
        case 5132: // Xib-
        case 5232: // Xib0
        case 5332: // Omegab-
            return true;
    }
    return false;
}

bool hasBeautyAncestor(GenParticle *particle, TClonesArray *particles)
{
    //check until beauty ancestor is found
    auto imother = particle->M1;
    if (imother == -1) return false;
    auto mother = (GenParticle *)particles->At(imother);
    auto pid = mother->PID;
    if (isBeauty(pid)) return true;
    return hasBeautyAncestor(mother, particles);

}

bool isCharm(int pdg) {
    switch (abs(pdg)) {
        case 411:  // D+
        case 421:  // D0
        case 431:  // Ds+
        case 4122: // Lambdac+
        case 4132: // Xic0
        case 4232: // Xic+
            return true;
    }
    return false;
}


bool hasCharmAncestor(GenParticle *particle, TClonesArray *particles)
{
    auto imother = particle->M1;
    if (imother == -1) return false;
    auto mother = (GenParticle *)particles->At(imother);
    auto pid = mother->PID;
    auto bCharm = false;
    bCharm = isCharm(pid);

    if (hasBeautyAncestor(mother, particles)) {bCharm = false;}
    return bCharm;
}


// TODO: write script to do this...
void bkg(const char *inputFile, const char *outputFile = "output.root")
{
// if(!inputFile) pritf("No input file specified.\n", );
    // Create chain of root trees
    TChain chain("Delphes");
    chain.Add(inputFile);

    // Create object of class ExRootTreeReader
    auto treeReader = new ExRootTreeReader(&chain);
    auto numberOfEntries = treeReader->GetEntries();

    // Get pointers to branches used in this analysis
    auto events = treeReader->UseBranch("Event");
    auto tracks = treeReader->UseBranch("Track");
    auto particles = treeReader->UseBranch("Particle");

    // smearer
    o2::delphes::TrackSmearer smearer;
    smearer.loadTable(11,   "./lutCovm.el.dat");
    smearer.loadTable(13,   "./lutCovm.mu.dat");
    smearer.loadTable(211,  "./lutCovm.pi.dat");
    smearer.loadTable(321,  "./lutCovm.ka.dat");
    smearer.loadTable(2212, "./lutCovm.pr.dat");

    // Event histograms
    auto nTracks = new TH1F("nTracks",";Tracks",1000,0,1000);
    auto nTracksEle = new TH1F("nTracksEle",";Tracks",1000,0,1000);
    auto nTracksPos = new TH1F("nTracksPos",";Tracks",1000,0,1000);
    // Track histograms
    auto hPt_trackEle = new TH1F("hPt_trackEle",";p_T (GeV/c)",200,0,20);
    auto hPt_trackPos = new TH1F("hPt_trackPos",";p_T (GeV/c)",200,0,20);
    // Pair histograms
    auto hM_Pt_sameMother = new TH2F("hM_Pt_sameMother",";m_{ee} (Gev/c^2);p_{T,ee} (GeV/c)",300.,0.,3.,400,0.,4.);
    auto hM_Pt_ULS = new TH2F("hM_Pt_ULS",";m_{ee} (Gev/c^2);p_{T,ee} (GeV/c)",300.,0,3.,400,0.,4.);
    auto hM_Pt_LSplus = new TH2F("hM_Pt_LSplus",";m_{ee} (Gev/c^2);p_{T,ee} (GeV/c)",300.,0,3.,400,0.,4.);
    auto hM_Pt_LSminus = new TH2F("hM_Pt_LSminus",";m_{ee} (Gev/c^2);p_{T,ee} (GeV/c)",300.,0,3.,400,0.,4.);

    std::vector<Track *> vecElectron,vecPositron;

    for (Int_t ientry = 0; ientry < numberOfEntries; ++ientry)
    {
        // Load selected branches with data from specified event
        treeReader->ReadEntry(ientry);
        nTracks->Fill(tracks->GetEntries());
        // loop over tracks and fill vectors for electrons and positrons
        for (Int_t itrack = 0; itrack < tracks->GetEntries(); ++itrack)
        {
            // get track and corresponding particle
            auto track = (Track *)tracks->At(itrack);
            // smear track if requested
            if (smear) if (!smearer.smearTrack(*track)) continue; // strange syntax, but works
            // kinatic cuts on tracks
            if (!kineCuts(track)) continue;
            // check the selection
            if (!isSelectedMC(track)) continue;

            // look only at electrons and fill track histos
            if (track->Charge < 0. )
            {
                vecElectron.push_back(track);
                hPt_trackEle->Fill(track->PT);
            }
            else if (track->Charge > 0. )
            {
                vecPositron.push_back(track);
                hPt_trackPos->Fill(track->PT);
            }
        }
        nTracksEle->Fill(vecElectron.size());
        nTracksPos->Fill(vecPositron.size());
        // pairing
        TLorentzVector LV1,LV2,LV;
        for (auto track1 : vecElectron)
        {
            auto particle1 = (GenParticle *)track1->Particle.GetObject();
            auto imother1 = particle1->M1;
            auto mother1 = imother1 != -1 ? (GenParticle *)particles->At(imother1) : (GenParticle *)nullptr;
            auto m1Pid = mother1->PID;
            LV1.SetPtEtaPhiM(track1->PT,track1->Eta,track1->Phi,eMass);
            for (auto track2 : vecPositron) {
                auto particle2 = (GenParticle *)track2->Particle.GetObject();
                auto imother2 = particle2->M1;
                auto mother2 = imother2 != -1 ? (GenParticle *)particles->At(imother2) : (GenParticle *)nullptr;
                auto m2Pid = mother2->PID;
                LV2.SetPtEtaPhiM(track2->PT,track2->Eta,track2->Phi,eMass);
                LV = LV1 + LV2;

                hM_Pt_ULS->Fill(LV.Mag(),LV.Pt()); // ULS spectrum
                if (mother1 == mother2) hM_Pt_sameMother->Fill(LV.Mag(),LV.Pt()); // same mother
            }
        }
        // Nested for loops for the calculation of the LS spectra
        for (auto track1 = vecPositron.begin(); track1!=vecPositron.end(); ++track1)
        {
            for (auto track2 = track1+1; track2!=vecPositron.end(); ++track2)
            {
                LV1.SetPtEtaPhiM((*track1)->PT,(*track1)->Eta,(*track1)->Phi,eMass);
                LV2.SetPtEtaPhiM((*track2)->PT,(*track2)->Eta,(*track2)->Phi,eMass);
                LV = LV1 + LV2;
                hM_Pt_LSplus->Fill(LV.Mag(),LV.Pt());
            }
        }

        for (auto track1 = vecElectron.begin(); track1!=vecElectron.end(); ++track1)
        {
            for (auto track2 = track1+1; track2!=vecElectron.end(); ++track2)
            {
                LV1.SetPtEtaPhiM((*track1)->PT,(*track1)->Eta,(*track1)->Phi,eMass);
                LV2.SetPtEtaPhiM((*track2)->PT,(*track2)->Eta,(*track2)->Phi,eMass);
                LV = LV1 + LV2;
                hM_Pt_LSminus->Fill(LV.Mag(),LV.Pt());
            }
        }
        vecElectron.clear();
        vecPositron.clear();
    }
    auto fout = TFile::Open(outputFile, "RECREATE");
    nTracks->Write();
    nTracksEle->Write();
    nTracksPos->Write();
    hPt_trackEle->Write();
    hPt_trackPos->Write();
    hM_Pt_sameMother->Write();
    hM_Pt_ULS->Write();
    hM_Pt_LSplus->Write();
    hM_Pt_LSminus->Write();
    fout->Close();

}
