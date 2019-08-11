#include "Timer.h"
#include "TimerSolt.h"
#include "FunctionNetMsg.h"

#include <iostream>

using namespace hudp;

#define EMPTY_WAIT  60000 // timer is empty. wait 1 min 

CTimer::CTimer() : _wait_time(EMPTY_WAIT) {
    
}

CTimer::~CTimer() {
    Stop();
    _notify.notify_one();
    Join();
}

uint64_t CTimer::AddTimer(uint32_t ms, CTimerSolt* ti) {
    _time_tool.Now();
    uint64_t expiration_time = ms + _time_tool.GetMsec();

    std::unique_lock<std::mutex> lock(_mutex);
    auto iter = _timer_map.find(expiration_time);
    ti->SetNext(nullptr);
    // add to timer map
    if (iter == _timer_map.end()) {
        _timer_map[expiration_time] = ti;

    // add same time
    } else {
        // check same item
        CTimerSolt* tmp = iter->second;
        while (tmp->GetNext()) {
            // the same item
            if (tmp == ti) {
                return _time_tool.GetMsec();
            } else {
                tmp = tmp->GetNext();
            }
        }
        // the same item
        if (tmp == ti) {
            return _time_tool.GetMsec();
        }

        // add to list
        tmp->SetNext(ti);
    }
    ti->_timer_id = expiration_time;
    // wake up timer thread
    if (ms < _wait_time) {
        _notify.notify_one();
    }
    return _time_tool.GetMsec();
}

void CTimer::RemoveTimer(CTimerSolt* ti) {
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _timer_map.erase(ti->_timer_id);
        ti->_timer_id = 0;
    }
    _notify.notify_one();
}

uint64_t CTimer::GetTimeStamp() {
    _time_tool.Now();
    return _time_tool.GetMsec();
}

void CTimer::Run() {
    std::vector<CTimerSolt*> timer_vec;
    std::map<uint64_t, CTimerSolt*>::iterator iter;
    CTimerSolt* cur_solt = nullptr;
    bool timer_out = false;
    while (!_stop) {
        {
            cur_solt = nullptr;
            timer_out = false;
            std::unique_lock<std::mutex> lock(_mutex);
            if (_timer_map.empty()) {
                _wait_time = EMPTY_WAIT;

            } else {
                iter = _timer_map.begin();
                _time_tool.Now();
                cur_solt = iter->second;
                if (iter->first > _time_tool.GetMsec()) {
                    _wait_time = iter->first - _time_tool.GetMsec();

                } else {
                    _wait_time = 0;
                    timer_out = true;
                }
            }
            if (_wait_time > 0) {
                timer_out = _notify.wait_for(lock, std::chrono::milliseconds(_wait_time)) == std::cv_status::timeout;
            }

            _time_tool.Now();
            
            // if timer out
            if (timer_out && cur_solt && cur_solt->_timer_id > 0 && cur_solt->_timer_id <= _time_tool.GetMsec()) {
                while (cur_solt) {
                    timer_vec.push_back(cur_solt);
                    cur_solt = cur_solt->GetNext();
                }
                _timer_map.erase(iter);

            } else if (timer_out && cur_solt && cur_solt->_timer_id == 0 && iter != _timer_map.end()) {
                _timer_map.erase(iter);
            }
        }

        if (timer_vec.size() > 0) {
            for (size_t i = 0; i < timer_vec.size(); i++) {
                timer_vec[i]->OnTimer();
            }
            timer_vec.clear();
        }
    }
}