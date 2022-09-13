# Copyright 2009 Canonical Ltd.
#
# This file is part of desktopcouch-contacts.
#
#  desktopcouch is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License version 3
# as published by the Free Software Foundation.
#
# desktopcouch is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with desktopcouch.  If not, see <http://www.gnu.org/licenses/>.
#
# Authors: Stuart Langridge <stuart.langridge@canonical.com>
#          Nicola Larosa <nicola.larosa@canonical.com>

"""Creating CouchDb-stored contacts for testing"""

import random, string, uuid

import desktopcouch.tests as test_environment
from desktopcouch.records.server import OAuthCapableServer
import desktopcouch

CONTACT_DOCTYPE = 'http://www.freedesktop.org/wiki/Specifications/desktopcouch/contact'
COUCHDB_SYS_PORT = 5984

FIRST_NAMES = ('Jack', 'Thomas', 'Oliver', 'Joshua', 'Harry', 'Charlie',
    'Daniel', 'William', 'James', 'Alfie', 'Grace', 'Ruby', 'Olivia',
    'Emily', 'Jessica', 'Sophie', 'Chloe', 'Lily', 'Ella', 'Amelia')
LAST_NAMES = ('Dalglish', 'Grobbelaar', 'Lawrenson', 'Beglin', 'Nicol',
    'Whelan', 'Hansen', 'Johnston', 'Rush', 'Molby', 'MacDonald', 'McMahon',
    'Penny', 'Leicester', 'Langley', 'Commodore', 'Touchstone', 'Fielding')
COMPANIES = ('Gostram', 'Grulthing', 'Nasform', 'Smarfish', 'Builsank')
STREET_TYPES = ('Street', 'Road', 'Lane', 'Avenue', 'Square', 'Park', 'Mall')
CITIES = ('Scunthorpe', 'Birmingham', 'Cambridge', 'Durham', 'Bedford')
COUNTRIES = ('England', 'Ireland', 'Scotland', 'Wales')

def head_or_tails():
    """Randomly return True or False"""
    return random.choice((False, True))

def random_bools(num_bools, at_least_one_true=True):
    """
    Return a sequence of random booleans. If at_least_one_true,
    guarantee at list one True value.
    """
    if num_bools < 2:
        raise RuntimeError('Cannot build a sequence smaller than two elements')
    bools = [head_or_tails() for __ in range(num_bools)]
    if at_least_one_true and not any(bools):
        bools[random.randrange(num_bools)] = True
    return bools

def random_string(length=10, upper=False):
    """
    Return a string, of specified length, of random lower or uppercase letters.
    """
    charset = string.uppercase if upper else string.lowercase
    return ''.join(random.sample(charset, length))

def random_postal_address():
    """Return something that looks like a postal address"""
    return '%d %s %s' % (random.randint(1, 100), random.choice(LAST_NAMES),
        random.choice(STREET_TYPES))

def random_email_address(
        first_name='', last_name='', company='', address_type='other'):
    """
    Return something that looks like an email address.
    address_type values: 'personal', 'work', 'other'.
    """
    # avoid including the dot if one or both names are missing
    pers_name = '.'.join([name for name in (first_name, last_name) if name])
    return {'personal': pers_name, 'work': company,
        }.get(address_type, random_string(5)) + '@example.com'

def random_postal_code():
    """Return something that looks like a postal code"""
    return '%s12 3%s' % (random_string(2, True), random_string(2, True))

def random_phone_number():
    """Return something that looks like a phone number"""
    return '+%s %s %s %s' % (random.randint(10, 99), random.randint(10, 99),
      random.randint(1000, 9999), random.randint(1000, 9999))

def random_birth_date():
    """Return something that looks like a birth date"""
    return '%04d-%02d-%02d' % (random.randint(1900, 2006),
        random.randint(1, 12), random.randint(1, 28))

def make_one_contact(maincount, doctype, app_annots):
    """Make up one contact randomly"""
    # Record schema
    fielddict = {'record_type': doctype, 'record_type_version': '1.0'}
    # Names
    # at least one of the three will be present
    has_first_name, has_last_name, has_company_name = random_bools(3)
    first_name = (random.choice(FIRST_NAMES) + str(maincount)
        ) if has_first_name else ''
    last_name = random.choice(LAST_NAMES) if has_last_name else ''
    company = (random.choice(COMPANIES) + str(maincount)
        ) if has_company_name else ''
    # Address places and types
    address_places, email_types, phone_places = [], [], []
    if has_first_name or has_last_name:
        for (has_it, seq, val) in zip(
                # at least one of the three will be present
                random_bools(3),
                (address_places, email_types, phone_places),
                ('home', 'personal', 'home')):
            if has_it:
                seq.append(val)
    if has_company_name:
        for (has_it, seq) in zip(
                # at least one of the three will be present
                random_bools(3),
                (address_places, email_types, phone_places)):
            if has_it:
                seq.append('work')
    for (has_it, seq) in zip(
            # none of the three may be present
            random_bools(3, at_least_one_true=False),
            (address_places, email_types, phone_places)):
        if has_it:
            seq.append('other')
    # Addresses
    addresses = {}
    for address_type in address_places:
        addresses[str(uuid.uuid4())] = {
            'address1': random_postal_address(), 'address2': '',
            'pobox': '', 'city': random.choice(CITIES),
            'state': '', 'postalcode': random_postal_code(),
            'country': random.choice(COUNTRIES), 'description': address_type}
    # Email addresses
    email_addresses = {}
    for email_address_type in email_types:
        email_addresses[str(uuid.uuid4())] = {
            'address': random_email_address(
                first_name, last_name, company, email_address_type),
            'description': email_address_type}
    # Phone numbers
    phone_numbers = {}
    for phone_number_type in phone_places:
        phone_numbers[str(uuid.uuid4())] = {
            'priority': 0, 'number': random_phone_number(),
            'description': phone_number_type}
    # Store data in fielddict
    if (has_first_name or has_last_name) and head_or_tails():
        fielddict['birth_date'] = random_birth_date()
    fielddict.update({'first_name': first_name, 'last_name': last_name,
        'addresses': addresses, 'email_addresses': email_addresses,
        'phone_numbers': phone_numbers, 'company': company})
    # Possibly add example application annotations
    if app_annots:
        fielddict['application_annotations'] = app_annots
    return fielddict

def create_many_contacts(num_contacts=10, host='localhost', port=None,
                         db_name='contacts', doctype=CONTACT_DOCTYPE,
                         app_annots=None, server_class=OAuthCapableServer):
    """Make many contacts and create their records"""
    if port is None:
        port = desktopcouch.find_port(ctx=test_environment.test_context)
    server_url = 'http://%s:%s/' % (host, port)
    server = server_class(server_url, ctx=test_environment.test_context)
    db = server[db_name] if db_name in server else server.create(db_name)
    record_ids = []
    for maincount in range(1, num_contacts + 1):
        # Make the contact
        fielddict = make_one_contact(maincount, doctype, app_annots)
        # Store data in CouchDB
        record_id = db.create(fielddict)
        record_ids.append(record_id)
    return record_ids
