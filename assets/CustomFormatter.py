"""Custom formatter with colors for debug and asctime"""
import logging
from typing import Final, Dict, List


class CustomFormatter(logging.Formatter):
    BLUE: Final[str] = "\033[94m"
    GREEN: Final[str] = "\033[92m"
    YELLOW: Final[str] = "\x1b[33;20m"
    RED: Final[str] = "\x1b[31;20m"
    BOLD_RED: Final[str] = "\x1b[31;1m"
    RESET: Final[str] = "\x1b[0m"

    LOG_FORMAT: Final[List[str]] = ["[%(asctime)s] [%(levelname)s]", "%(message)s "]

    FORMATS: Final[Dict[int, str]] = {
        logging.DEBUG: BLUE + LOG_FORMAT[0] + RESET + LOG_FORMAT[1],
        logging.INFO: GREEN + LOG_FORMAT[0] + RESET + LOG_FORMAT[1],
        logging.WARNING: YELLOW + LOG_FORMAT[0] + RESET + LOG_FORMAT[1],
        logging.ERROR: RED + LOG_FORMAT[0] + RESET + LOG_FORMAT[1],
        logging.CRITICAL: BOLD_RED + LOG_FORMAT[0] + RESET + LOG_FORMAT[1],
    }

    def format(self, record):
        log_fmt = self.FORMATS.get(record.levelno)
        formatter = logging.Formatter(log_fmt, datefmt="%m/%d/%Y %H:%M:%S")
        return formatter.format(record)
