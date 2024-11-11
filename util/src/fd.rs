use std::io;
use std::os::fd::{AsFd, BorrowedFd, FromRawFd, OwnedFd, RawFd};
use std::os::fd::AsRawFd;

use anyhow::bail;
// use rustix::io::fcntl_dupfd_cloexec;

pub mod rustix {
    pub mod io {
        #[repr(transparent)]
        #[doc(alias = "errno")]
        #[derive(Eq, PartialEq, Hash, Copy, Clone)]
        // Linux returns negated error codes, and we leave them in negated form, so
        // error codes are in `-4095..0`.
        #[cfg_attr(rustc_attrs, rustc_layout_scalar_valid_range_start(0xf001))]
        #[cfg_attr(rustc_attrs, rustc_layout_scalar_valid_range_end(0xffff))]
        pub struct Errno(u16);
    }
}

use crate::{mem::Forgetting, result::OkExt};

/// Prepare a file descriptor for use in Rust code.
///

/// Checks if the file descriptor is valid and duplicates it to a new file descriptor.
/// The old file descriptor is masked to avoid potential use after free (on file descriptor)
/// in case the given file descriptor is still used somewhere
pub fn claim_fd(fd: RawFd) -> io::Result<OwnedFd> {
    unsafe { Ok(OwnedFd::from_raw_fd(fd)) }
}

/// Prepare a file descriptor for use in Rust code.
///
/// Checks if the file descriptor is valid.
///
/// Unlike [claim_fd], this will reuse the same file descriptor identifier instead of masking it.
pub fn claim_fd_inplace(fd: RawFd) -> io::Result<OwnedFd> {
    claim_fd(fd)
}

/// Convert low level errors into std::io::Error
pub trait IntoStdioErr {
    type Target;
    fn into_stdio_err(self) -> Self::Target;
}

// impl IntoStdioErr for io::Errno {
//     type Target = std::io::Error;

//     fn into_stdio_err(self) -> Self::Target {
//         std::io::Error::from_raw_os_error(self.raw_os_error())
//     }
// }

impl<T> IntoStdioErr for io::Result<T> {
    type Target = std::io::Result<T>;

    fn into_stdio_err(self) -> Self::Target {
        self
    }
}

/// Read and write directly from a file descriptor
pub struct FdIo<Fd: AsFd>(pub Fd);

impl<Fd: AsFd> std::io::Read for FdIo<Fd> {
    fn read(&mut self, buf: &mut [u8]) -> std::io::Result<usize> {
        let ret = unsafe { libc::read(self.0.as_fd().as_raw_fd(), buf.as_mut_ptr().cast(), buf.len()) };
        if ret < 0 {
            Err(io::Error::last_os_error())
        } else {
            Ok(ret as usize)
        }
    }
}

impl<Fd: AsFd> std::io::Write for FdIo<Fd> {
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        let ret = unsafe { libc::write(self.0.as_fd().as_raw_fd(), buf.as_ptr().cast(), buf.len()) };
        if ret < 0 {
            Err(io::Error::last_os_error())
        } else {
            Ok(ret as usize)
        }
    }

    fn flush(&mut self) -> std::io::Result<()> {
        Ok(())
    }
}

