#pragma once

int portalOnceObserver = -1;
std::vector<int> portalOnceCrossed;

bool portalOnceRefresh() {
    if (portalOnceObserver == MYID) return true;
    portalOnceObserver = MYID;
    portalOnceCrossed.clear();
    return false;
}

void portalOnceConfirm(int key) {
    if (portalOnceObserver != MYID || key < 0 || key >= NC * 4 ||
        edgeT[key] != E_PORTAL || learnedExit[key] != HERE || edgePid[key] < 0) return;
    int pid = edgePid[key];
    if (std::find(portalOnceCrossed.begin(), portalOnceCrossed.end(), pid) == portalOnceCrossed.end())
        portalOnceCrossed.push_back(pid);
}

bool portalOnceUsed(int pid) {
    return portalOnceObserver == MYID &&
           std::find(portalOnceCrossed.begin(), portalOnceCrossed.end(), pid) != portalOnceCrossed.end();
}
