use localtime::{self, get_current_time, time_to_info};
use std::os::raw::c_long;

#[derive(Clone, Copy)]
#[derive(PartialEq)]
pub enum LogLevel {
    Info,
    Warning,
    Error,
}

fn __log_level_color(level: LogLevel) -> String {
    match level {
        LogLevel::Info => return String::from("\x1b[32m"),
        LogLevel::Warning => return String::from("\x1b[33m"),
        LogLevel::Error => return String::from("\x1b[31m"),
    }
}

fn __log_level_name(level: LogLevel) -> String {
    match level {
        LogLevel::Info => return String::from("Info"),
        LogLevel::Warning => return String::from("Warning"),
        LogLevel::Error => return String::from("Error"),
    }
}

pub struct LogNode {
    pub source: String,
    pub content: String,
    pub level: LogLevel,
    pub time_stamp: c_long,
    pub next: Option<Box<LogNode>>,
}

pub struct LogList {
    pub head: Option<Box<LogNode>>,
    pub tail: *mut LogNode,
}

impl LogNode {
    pub fn new(source: String, content: String, level: LogLevel,
        time: c_long) ->Self {
        LogNode {
            source: source, content: content,
            level: level, time_stamp: time,
            next: None
        }
    }
}

impl LogList {
    pub fn new() -> Self {
        LogList {head: None, tail: std::ptr::null_mut()}
    }
    pub fn add(&mut self, level: LogLevel, source: String,
        content: String) {
        let time = get_current_time().unwrap();
        let mut new_node = Box::new(LogNode::new(source, content, level, time));
        let new_node_ptr: *mut LogNode = &mut *new_node;
        if self.head.is_none() {
            self.head = Some(new_node);
            self.tail = new_node_ptr;
        } else {
            unsafe {
                (*self.tail).next = Some(new_node);
            }
        }
    }
    pub fn as_string(&mut self) -> String {
        let mut result = String::new();
        let mut previus_level: Option<LogLevel> = None;
        let mut previus_source = String::new();
        let mut current = self.head.as_ref();
        while let Some(node) = current {
            if previus_source != node.source || previus_level.as_ref() == Some(&node.level) {
                unsafe {
                    let tm = *time_to_info(node.time_stamp).unwrap();
                    result += &format!("\n[{}{}\x1b[0m] [\x1b[32m{}/{}/{} {}:{}:{}\x1b[0m] {}\n",
                        __log_level_color(node.level), __log_level_name(node.level),
                        tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour,
                        tm.tm_min, tm.tm_sec, node.source);
                }
                previus_source.clear();
                previus_source += &node.source;
                previus_level = Some(node.level);
            }
            result += &format!("{}\n", node.content);
            current = node.next.as_ref();
        }
        result
    }

}

#[no_mangle]
pub extern "C" fn test_log() {
    let mut logger = LogList::new();
    logger.add(LogLevel::Info, "Rust Code".to_string(), "Sistema de log desde rust".to_string());
    print!("{}", logger.as_string());
}
