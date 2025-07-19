use std::time::{SystemTime, UNIX_EPOCH};
use std::os::raw::{c_int, c_long};

#[repr(C)]
#[allow(non_camel_case_types)]
#[derive(Clone, Copy)]
pub struct tm {
    pub tm_sec: c_int,
    pub tm_min: c_int,
    pub tm_hour: c_int,
    pub tm_mday: c_int,
    pub tm_mon: c_int,
    pub tm_year: c_int,
    pub tm_wday: c_int,
    pub tm_yday: c_int,
    pub tm_isdst: c_int,
}

extern "C" {
    fn localtime(timep: *const c_long) -> *const tm;
}

pub fn get_current_time() -> Option<c_long> {
    let now = SystemTime::now();
    match now.duration_since(UNIX_EPOCH) {
        Ok(duration) => Some(duration.as_secs() as c_long),
        Err(_) => None,
    }
}

pub fn time_to_info(time: c_long) -> Option<*const tm> {
    unsafe {
        let time_info = localtime(&time);
        if time_info.is_null() {
            return None;
        }
        Some(time_info)
    }
}
