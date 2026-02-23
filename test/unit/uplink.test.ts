/**
 * @file uplink.test.ts
 * @brief Unit tests for the Uplink class
 */

import { Uplink, AccessResultStruct } from '../../src';

describe('Uplink Class', () => {
    describe('constructor', () => {
        it('should create an Uplink instance', () => {
            const uplink = new Uplink();
            expect(uplink).toBeInstanceOf(Uplink);
        });
    });

    describe('parseAccess input validation', () => {
        it('should throw TypeError for empty access grant', async () => {
            const uplink = new Uplink();
            await expect(uplink.parseAccess('')).rejects.toThrow(TypeError);
        });

        it('should throw TypeError for non-string input', async () => {
            const uplink = new Uplink();
            // @ts-expect-error - Testing runtime type checking
            await expect(uplink.parseAccess(123)).rejects.toThrow(TypeError);
        });
    });

    describe('requestAccessWithPassphrase input validation', () => {
        it('should throw TypeError for empty satellite address', async () => {
            const uplink = new Uplink();
            await expect(
                uplink.requestAccessWithPassphrase('', 'api-key', 'passphrase')
            ).rejects.toThrow(TypeError);
        });

        it('should throw TypeError for empty API key', async () => {
            const uplink = new Uplink();
            await expect(
                uplink.requestAccessWithPassphrase('satellite.example.com:7777', '', 'passphrase')
            ).rejects.toThrow(TypeError);
        });

        it('should throw TypeError for empty passphrase', async () => {
            const uplink = new Uplink();
            await expect(
                uplink.requestAccessWithPassphrase('satellite.example.com:7777', 'api-key', '')
            ).rejects.toThrow(TypeError);
        });
    });
});

describe('AccessResultStruct Class', () => {
    describe('class structure', () => {
        it('should be a class', () => {
            expect(typeof AccessResultStruct).toBe('function');
        });

        it('should have expected methods', () => {
            // Can't instantiate without a valid handle, but can check prototype
            expect(typeof AccessResultStruct.prototype.openProject).toBe('function');
            expect(typeof AccessResultStruct.prototype.configOpenProject).toBe('function');
            expect(typeof AccessResultStruct.prototype.serialize).toBe('function');
            expect(typeof AccessResultStruct.prototype.satelliteAddress).toBe('function');
            expect(typeof AccessResultStruct.prototype.share).toBe('function');
            expect(typeof AccessResultStruct.prototype.overrideEncryptionKey).toBe('function');
        });
    });
});
