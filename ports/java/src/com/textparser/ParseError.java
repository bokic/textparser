package com.textparser;

/**
 * Parse failure carrying the source span that triggered it.
 *
 * Mirrors the C error reporting contract ({@code textparser_parse_error},
 * {@code textparser_parse_error_position}, {@code textparser_parse_error_length}):
 * a message plus a half-open {@code [position, position + length)} span. Internal
 * errors (bad definition, allocation-style failures) use a zero length.
 */
public class ParseError extends RuntimeException {
    private final int position;
    private final int length;

    public ParseError(String message, int position, int length) {
        super(message);
        this.position = position;
        this.length = Math.max(0, length);
    }

    /** Character-unit offset where the error begins. */
    public int getPosition() {
        return position;
    }

    /** Length of the offending span in character units (may be 0). */
    public int getLength() {
        return length;
    }

    @Override
    public String toString() {
        return super.getMessage() + " at position " + position + " (length " + length + ")";
    }
}
