use core::ffi::c_void;

#[repr(C)]
pub struct BasicInstance {
    pub inst_type: u32,
    pub inst_data: *mut c_void,
    pub as_string: Option<extern "C" fn(BasicInstance) -> *const char>,
}

#[no_mangle]
pub extern "C" fn instance_new() -> *mut BasicInstance {
    std::ptr::null_mut()
}
