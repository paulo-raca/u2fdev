# (Ab)using U2F for biometric authentication.

## Background
I worked several years with fingerprint recognition SDKs, and back in the day, there was only one big issue.

Not the fingerprint scanners -- Despite being mostly overpriced, sometimes crappy, 
they all do work --, and not with the matching algorithms either -- as long as you 
don't expect CSI performance, it works surprisingly well.

No, the biggest problem has always been integrating biometrics with the target applications.
With no (good) standards in sight, every vendor had it's own APIs -- Many of which were crappy, 
unstable, non-portable, etc -- most applications would never integrate support for biometrics,
and on those applications that did, the developers would usually be stuck to one vendor's SDK and
it's limited number of fingerprint scanner models.

To make things worse, everything is moving to the Web, and getting biometry to work on a webpage is 
quite challenging. With no browser support, hacks are necessary: 
Back in the day, I used a signed Java Applet. 
After a scary warning message would be allowed to escape the Java sandbox and load a native library with the SDK which talked to the fingerprint scanners. It worked, but... 😭

Nevertheless, I changed jobs and forgot all about it for years. Until I heard about U2F.

## 2-Factor authentication

Banks have been using security tokens for some time now, but it was only in the last few years 
that 2-factor authentication became mainstream.

As the industry started pushing for 2-factor authentication, the Fido U2F standard was created. 

And since I first heard about it, I think it is a perfect platform to implement biometric authentication.

## Fido U2F overview

I recently had a few weeks off and decided to finally take this project.

First off, [reading the specs](https://fidoalliance.org/download/), I learned that the standard 
is really simple, cryptographically strong, and allows for any U2F-compliant device to be used by
any U2F-enabled software. Importantly, it is well supported on several web browsers and mobile platforms.

Also, the protocol has only 2 operations:
- Register: A new key pair is created, the public key and a handle are returned to the application. 
The private key is kept secret (duh!), but the device can use the handle to retrieved it.
- Authenticate: Given a handle, the device must retrieve the corresponding private key and use it to sign a challenge.

In either case, the user presence is required -- Otherwise the operation fails and the application keeps retrying 
until you are there.

Importantly, it is completely up to the implementation how check for user presence.
It's usually a button, but I wanted to use fingerprint scanners.

## Biometric U2F

First off, since fingerprint scanners are usually flashy and blinky, I decided to keep them off most of the time. 

Whenever I get a request to register or authenticate, I'll turn the fingerprint scanner on, and after a 
few seconds without any request they will be turned back off. This behavior draws the user attention at the right time.

A minor side effect is that the first few requests will always fail the user presence check, 
until the fingerprint scanner has had the time to start up and captured a fingerprint.
But as long as the application keeps retrying, it is fine.

Once a fingerprint is placed on the scanner, the current fingerprint template is extracted.
After the fingerprint has been removed from the sensor, the fingerprint template is cleared.

During a registration, the fingerprint template is stored alongside the private key.

During verification, the fingerprint template used for registration is compared to the current fingerprint template. 
If they don't match, the user presence verification fails.

After any successfull request, the current fingerprint template is cleared, ensuring the user needs to make a new 
fingerprint capture for the next operation.

This works nicely, and performs exactly as U2F is meant to be used. But there is another important scenario:

## Abusing U2F: Same account, multiple devices

While Biometric U2F is fun, a very real scenario isn't covered: Not everyone carries their own biometric U2F tokens around.

Governments, Banks, health insurers, etc, employ a large number of fingerprint scanners to verify people's 
identities -- usually for fraud prevention.

In these cases, the people being verified must be able to use any of the devices available -- No matter 
which burocratic counter or ATM machine they are at. While this is NOT what U2F was designed for, 
it can be tweaked to work on this scenario.

On most U2F devices, the handle is actually used as an encrypted storage for the private key. 
Each device has it's own distinct encryption key, therefore only the device that performed the 
registration can be used to perform the authentication.

To support the multiple-device scenario, we must embed both the private key and the fingerprint template on the handle.
The maximum handle size is 255 bytes, and the private key takes 32 of these, leaving a little over 200 bytes for the template.
While it doesn't leave much room to spare, it should be enough.

Given that we want all devices to be able to extract private keys and templates from the handles without publicly
exposing either private keys or templates, the handle will have to be encrypted by a private key shared among all the devices.

There is another implementation detail that must be accounted for: There is an authentication counter returned on every 
successful authentication. It must be always-increasing, otherwise the client will suspect the device was cloned.
Since the counter must be monotonically-increasing and synchronized between devices, we can just use a timestamp. 
-- IMHO, the authentication counter doesn't provide much of a protection, the clone just needs to return very
large numbers. I'd rather remove it from the specs.

Such device works seamlessly on any U2F-enabled service, and it's awesome :)


Unfortunately, it comes with a few drawbacks:
- You must trust the device manufacturer with full access to all private keys and fingerprint templates on any device.
- You will be stuck with a single device manufacturer for all your installations.
- If a single device gets hacked, all private keys and all fingerprint templates become public.

Fortunately, that's not worse than what most people are doing with biometrics 😓.

Anyway, I'd love to hear better approaches. (Remember, U2F allows for vendor extensions!)
