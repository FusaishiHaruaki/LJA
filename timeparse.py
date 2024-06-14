import re
import datetime

def parseTimings(file):
    phases = []
    func_times = {}
    phase_times = {}
    with open(file, "r") as f:
        lines = f.readlines()
        matcher = re.compile("^(.+)\(.* time: (\d+:\d+:\d+):(\d+)")
        matcher2 = re.compile("^(.+) time: (\d+:\d+:\d+):(\d+)")
        for l in lines:
            if l.count("time") > 1 or l.count("Gb") > 1:
                continue
            if '(' in l:
                search = matcher.search(l)
            else:
                search = matcher2.search(l)
            if search != None:
                m = search.groups()
                # print(m)
                func, elapsed_time, milliseconds = m
                date_obj = datetime.datetime.strptime(elapsed_time, "%H:%M:%S")
                hour = date_obj.hour
                minute = date_obj.minute
                second = date_obj.second
                delta = datetime.timedelta(hours=hour, minutes=minute, seconds=second)
                milliseconds = delta.seconds * 1000 + int(milliseconds)
                if func not in func_times:
                    func_times[func] = milliseconds
                else:
                    func_times[func] += milliseconds
                if func not in phase_times:
                    phase_times[func] = milliseconds
                else:
                    phase_times[func] += milliseconds
            if "Phase" in l or "AlternativeCorrection" in func:
                phase_times = dict(reversed(sorted(phase_times.items(), key=lambda x: x[1])))
                phases.append(phase_times)
                phase_times = {}
    func_times = dict(reversed(sorted(func_times.items(), key=lambda x: x[1])))
    return func_times, phases